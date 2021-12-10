/** @file ssm-scheduler.c
 *  @brief Implementation of the SSM runtime scheduler.
 *
 *  Contains a priority queue implemented using a binary heap.
 *
 *  When `SSM_DEBUG` is defined, relevant symbols are exported for whitebox
 *  testing.
 *
 *  @author Stephen Edwards (sedwards-lab)
 *  @author John Hui (j-hui)
 */
#include <ssm-internal.h>

/**
 * defgroup queue Priority queue
 */

#ifdef SSM_DEBUG
#define SSM_STATIC
#define SSM_STATIC_INLINE
#else
#define SSM_STATIC static
#define SSM_STATIC_INLINE static inline
#endif

/**
 * @addtogroup queue
 * @{
 */

#ifndef SSM_EVENT_QUEUE_SIZE
/** @brief Size of the event queue; override as necessary. */
#define SSM_EVENT_QUEUE_SIZE 2048
#endif

#ifndef SSM_ACT_QUEUE_SIZE
/** @brief Size of the activation record queue; override as necessary. */
#define SSM_ACT_QUEUE_SIZE 1024
#endif

/** @brief Extra entries at the start of a binary heap. */
#define SSM_QUEUE_HEAD 1

/** @brief Queue index type; index of 1 is the first element. */
typedef uint16_t q_idx_t;

/** @brief Event queue, used to track and schedule events between instants.
 *
 *  Managed as a binary heap, sorted by @a later_time.
 *
 *  Since the first element (#SSM_QUEUE_HEAD) is unusued,
 *  event_queue[event_queue_len] is the last element in the queue provided
 *  event_queue_len is non-zero.
 */
SSM_STATIC ssm_sv_t *event_queue[SSM_EVENT_QUEUE_SIZE + SSM_QUEUE_HEAD];

/** @brief The number of elements in #event_queue. */
SSM_STATIC q_idx_t event_queue_len = 0;

/** @brief Activation record queue, for scheduling at each instant.
 *
 * Managed as a binary heap sorted by @a priority.
 */
SSM_STATIC ssm_act_t *act_queue[SSM_ACT_QUEUE_SIZE + SSM_QUEUE_HEAD];

/** @brief the number of elements in #act_queue. */
SSM_STATIC q_idx_t act_queue_len = 0;

/** @brief The current model time.
 *
 *  Read with ssm_now(); user programs should not manipuate this directly.
 *
 *  This starts out initialized to 0; can be reset by ssm_reset().
 *
 *  ssm_set_now() (and its caller ssm_tick()) advances #now monotonically.
 */
SSM_STATIC ssm_time_t now = 0L;

/** @brief Percolate act queue hole up, to find where to put a new entry.
 *
 *  Starting at the hole, walk up toward the root of the tree, copying parent to
 *  child until we find where we can put the new activation record.
 *
 *  @param hole  index of the hole that needs to be filled.
 *  @param act   activation record that needs to be placed in the heap.
 */
SSM_STATIC_INLINE
void act_queue_percolate_up(q_idx_t hole, ssm_act_t *act) {
  SSM_ASSERT(hole >= SSM_QUEUE_HEAD && hole <= act_queue_len);
  ssm_priority_t priority = act->priority;
  for (; hole > SSM_QUEUE_HEAD && priority < act_queue[hole >> 1]->priority;
       hole >>= 1)
    act_queue[hole] = act_queue[hole >> 1];
  act_queue[hole] = act;
  act->scheduled = true;
}

/** @brief Percolate act queue hole down, to find where to put a new entry.
 *
 *  Starting at the hole, walk down towards the leaves of the tree, moving the
 *  earlier child to the parent and repeating the process on that child. This
 *  makes the parent earlier than both children. Stop when the activation record
 *  we're trying to place has a lower priority than either children.
 *
 *  @param hole  where to place the given activation record.
 *  @param act   activation record to be placed in the queue.
 */
SSM_STATIC_INLINE
void act_queue_percolate_down(q_idx_t hole, ssm_act_t *act) {
  SSM_ASSERT(hole >= SSM_QUEUE_HEAD && hole <= act_queue_len);
  ssm_priority_t priority = act->priority;
  for (;;) {
    // Find the earlier of the two children
    q_idx_t child = hole << 1; // Left child
    if (child > act_queue_len)
      break; // The parent was a leaf
    if (child + 1 <= act_queue_len &&
        act_queue[child + 1]->priority < act_queue[child]->priority)
      child++; // Right child is earlier than the left

    if (priority < act_queue[child]->priority)
      break; // Earlier child is later than what we're inserting
    act_queue[hole] = act_queue[child];
    hole = child;
  }
  act_queue[hole] = act;
}

/** @brief Percolate event queue hole up, to find where to put a new entry.
 *
 *  Starting at the hole, walk up toward the root of the tree, copying parent to
 *  child until we find where we can put the new event.
 *
 *  The @a later_time field of @a var should already be set to the value it is
 *  sorted on.
 *
 *  @param hole index of the hole, from 1 to #event_queue_len, inclusive.
 *  @param var  event to place in the queue.
 */
SSM_STATIC_INLINE
void event_queue_percolate_up(q_idx_t hole, ssm_sv_t *var) {
  SSM_ASSERT(hole >= SSM_QUEUE_HEAD && hole <= event_queue_len);
  ssm_time_t later = var->later_time;
  for (; hole > SSM_QUEUE_HEAD && later < event_queue[hole >> 1]->later_time;
       hole >>= 1)
    event_queue[hole] = event_queue[hole >> 1];
  event_queue[hole] = var;
}

/** @brief Percolate event queue hole down, to find where to put a new entry.
 *
 *  Starting at the hole, walk down towards the leaves of the tree, moving the
 *  earlier child to the parent and repeating the process on that child.  This
 *  makes the parent earlier than both children. Stop when the event we're
 *  trying to place is earlier than both of the children.
 *
 *  @param hole   where to place @a event.
 *  @param event  event to place in the queue.
 */
SSM_STATIC_INLINE
void event_queue_percolate_down(q_idx_t hole, ssm_sv_t *event) {
  SSM_ASSERT(hole >= SSM_QUEUE_HEAD && hole <= event_queue_len);
  ssm_time_t later = event->later_time;
  for (;;) {
    // Find the earlier of the two children
    q_idx_t child = hole << 1; // Left child
    if (child > event_queue_len)
      break; // The parent was a leaf
    if (child + 1 <= event_queue_len &&
        event_queue[child + 1]->later_time < event_queue[child]->later_time)
      child++; // Right child is earlier than the left

    if (later < event_queue[child]->later_time)
      break; // Earlier child is later than what we're inserting
    event_queue[hole] = event_queue[child];
    hole = child;
  }
  event_queue[hole] = event;
}

/** @brief Determine the index of an already scheduled event.
 *
 *  This is an inefficient linear search, so it's not meant to be used often.
 *  This is in preparation for either removing or rescheduling an event in the
 *  queue. The function should only be called on a variable that is in the
 *  queue.
 *
 *  @param var  the variable being searched for.
 */
SSM_STATIC_INLINE
q_idx_t find_queued_event(ssm_sv_t *var) {
  SSM_ASSERT(var->later_time != SSM_NEVER); // Should be in the queue
  for (q_idx_t i = SSM_QUEUE_HEAD; i <= event_queue_len; i++)
    if (event_queue[i] == var)
      return i;
  SSM_ASSERT(0); // Should have found the variable if it was marked as queued
  return 0;
}

#ifdef SSM_DEBUG
#include <assert.h>

/** @brief Assert the event queue is well-formed. */
void event_queue_consistency_check(void) {
  if (event_queue_len == 0)
    return;

  assert(event_queue_len <= SSM_EVENT_QUEUE_SIZE); // No overflow

  for (q_idx_t i = SSM_QUEUE_HEAD; i <= event_queue_len; i++) {
    assert(event_queue[i]); // Events should be valid
    assert(event_queue[i]->later_time !=
           SSM_NEVER); // Queue events should have valid time
    q_idx_t child = i << 1;
    if (child <= event_queue_len) {
      assert(event_queue[child]);
      assert(event_queue[child]->later_time >= event_queue[i]->later_time);
      if (++child <= event_queue_len) {
        assert(event_queue[child]);
        assert(event_queue[child]->later_time >= event_queue[i]->later_time);
      }
    }
  }
}

/** @brief Assert the activation record queue is well-formed. */
void act_queue_consistency_check(void) {
  if (act_queue_len == 0)
    return;

  assert(act_queue_len <= SSM_ACT_QUEUE_SIZE); // No overflow

  for (q_idx_t i = SSM_QUEUE_HEAD; i <= act_queue_len; i++) {
    assert(act_queue[i]);            // Acts should be valid
    assert(act_queue[i]->scheduled); // If it's in the queue, it should say so
    q_idx_t child = i << 1;
    if (child <= act_queue_len) {
      assert(act_queue[child]);
      assert(act_queue[child]->priority >= act_queue[i]->priority);
      if (++child <= act_queue_len) {
        assert(act_queue[child]);
        assert(act_queue[child]->priority >= act_queue[i]->priority);
      }
    }
  }
}
#endif

/** @} */

ssm_time_t ssm_now(void) { return now; }

struct ssm_time *ssm_new_time(ssm_time_t time) {
  struct ssm_mm *mm = ssm_new_builtin(SSM_TIME_T);
  struct ssm_time *t = container_of(mm, struct ssm_time, mm);
  t->time = time;
  return t;
}

ssm_act_t *ssm_enter(size_t size, ssm_stepf_t step, ssm_act_t *parent,
                     ssm_priority_t priority, ssm_depth_t depth) {
  ssm_act_t *act = malloc(size); // TODO: use something else
  *act = (ssm_act_t){
      .step = step,
      .caller = parent,
      .pc = 0,
      .children = 0,
      .priority = priority,
      .depth = depth,
      .scheduled = false,
  };
  ++parent->children;
  return act;
}

void ssm_leave(ssm_act_t *act, size_t size) {
  ssm_act_t *parent = act->caller;
  free(act); // TODO:
  if (--parent->children == 0)
    parent->step(parent);
}

void ssm_activate(ssm_act_t *act) {
  if (act->scheduled)
    return; // Don't activate an already activated routine

  q_idx_t hole = ++act_queue_len;

  if (act_queue_len > SSM_ACT_QUEUE_SIZE)
    SSM_THROW(SSM_EXHAUSTED_ACT_QUEUE);

  act_queue_percolate_up(hole, act);
}

ssm_sv_t *ssm_new_sv(ssm_value_t val) {
  struct ssm_mm *mm = ssm_new_builtin(SSM_SV_T);
  ssm_sv_t *sv = container_of(mm, ssm_sv_t, mm);
  sv->triggers = NULL;
  sv->later_time = SSM_NEVER;
  sv->last_updated = SSM_NEVER;
  return sv;
}

void ssm_assign(ssm_sv_t *var, ssm_priority_t prio, ssm_value_t value) {
  var->value = value;
  var->last_updated = now;
  for (ssm_trigger_t *trig = var->triggers; trig; trig = trig->next)
    if (trig->act->priority > prio)
      ssm_activate(trig->act);
}

void ssm_later(ssm_sv_t *var, ssm_time_t later, ssm_value_t value) {
  if (later < now)
    // later must be in the future
    SSM_THROW(SSM_INVALID_TIME);

  var->later_value = value;

  if (var->later_time == SSM_NEVER) {
    // Variable does not have a pending event: add it to the queue
    q_idx_t hole = ++event_queue_len;

    if (event_queue_len > SSM_EVENT_QUEUE_SIZE)
      SSM_THROW(SSM_EXHAUSTED_EVENT_QUEUE);

    var->later_time = later;
    event_queue_percolate_up(hole, var);

  } else {
    // Variable has a pending event: reposition the event in the queue
    // as appropriate
    q_idx_t hole = find_queued_event(var);

    var->later_time = later;

    if (hole == SSM_QUEUE_HEAD || event_queue[hole >> 1]->later_time < later)
      event_queue_percolate_down(hole, var);
    else
      event_queue_percolate_up(hole, var);
  }
}

void ssm_sensitize(ssm_sv_t *var, ssm_trigger_t *trigger) {
  // Point us to the first element
  trigger->next = var->triggers;

  if (var->triggers)
    // Make first element point to us
    var->triggers->prev_ptr = &trigger->next;

  // Insert us at the beginning
  var->triggers = trigger;

  // Our previous is the variable
  trigger->prev_ptr = &var->triggers;
}

void ssm_desensitize(ssm_trigger_t *trigger) {
  // Tell predecessor to skip us
  *trigger->prev_ptr = trigger->next;

  if (trigger->next)
    // Tell successor its predecessor is our predecessor
    trigger->next->prev_ptr = trigger->prev_ptr;
}

void ssm_unschedule(ssm_sv_t *var) {
  if (var->later_time != SSM_NEVER) {
    q_idx_t hole = find_queued_event(var);
    var->later_time = SSM_NEVER;
    ssm_sv_t *moved_var = event_queue[event_queue_len--];
    if (hole < SSM_QUEUE_HEAD + event_queue_len) {
      // Percolate only if removal led to a hole in the queue; no need to do
      // this if we happened to remove the last element of the queue.
      if (hole == SSM_QUEUE_HEAD ||
          event_queue[hole >> 1]->later_time < moved_var->later_time)
        event_queue_percolate_down(hole, moved_var);
      else
        event_queue_percolate_up(hole, moved_var);
    }
  }
}

ssm_time_t ssm_next_event_time(void) {
  return event_queue_len ? event_queue[SSM_QUEUE_HEAD]->later_time : SSM_NEVER;
}

void ssm_reset(void) {
  now = 0L;
  event_queue_len = 0;
  act_queue_len = 0;
}

void ssm_set_now(ssm_time_t next) {
  SSM_ASSERT(now < next); // No time-traveling!
  SSM_ASSERT(event_queue_len == 0 ||
             next <= event_queue[SSM_QUEUE_HEAD]->later_time);
  now = next;
}

void ssm_update(ssm_sv_t *sv) {
  SSM_ASSERT(now == sv->later_time);
  sv->value = sv->later_value;
  sv->last_updated = now;
  sv->later_time = SSM_NEVER;
  for (ssm_trigger_t *trigger = sv->triggers; trigger; trigger = trigger->next)
    ssm_activate(trigger->act);
}

void ssm_tick(void) {
  SSM_ASSERT(act_queue_len == 0 || ssm_next_event_time() == SSM_NEVER ||
             now <= ssm_next_event_time());

  if (act_queue_len == 0 && event_queue_len > 0)
    ssm_set_now(ssm_next_event_time());

  // Update every variable in the event queue at the current time.
  while (event_queue_len > 0 &&
         event_queue[SSM_QUEUE_HEAD]->later_time == now) {
    ssm_sv_t *sv = event_queue[SSM_QUEUE_HEAD];
    ssm_update(sv);

    // Remove the top event from the queue by inserting the last element in the
    // queue at the front and percolating it toward the leaves.
    ssm_sv_t *to_insert = event_queue[event_queue_len--];

    if (event_queue_len) /* Only percolate if there are other events left */
      event_queue_percolate_down(SSM_QUEUE_HEAD, to_insert);
  }

  while (act_queue_len > 0) {
    ssm_act_t *to_run = act_queue[SSM_QUEUE_HEAD];
    to_run->scheduled = false;

    // Remove the top activation record from the queue by inserting the last
    // element in the queue at the front and percolating it down.
    ssm_act_t *to_insert = act_queue[act_queue_len--];

    if (act_queue_len)
      act_queue_percolate_down(SSM_QUEUE_HEAD, to_insert);

    to_run->step(to_run);
  }
}
