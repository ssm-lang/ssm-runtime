/** @file ssm-internal.h
 *  @brief The internal interface of the SSM runtime.
 *
 *  This header file contains definitions and declarations that should not be
 *  exposed to user code.
 *
 *  @author Stephen Edwards (sedwards-lab)
 *  @author John Hui (j-hui)
 */
#ifndef _SSM_SCHED_H
#define _SSM_SCHED_H

#include <ssm.h>

/** @brief Throw an internal error.
 *
 *  @param cond the condition to assert.
 */
#define SSM_ASSERT(cond)                                                       \
  do                                                                           \
    if (cond)                                                                  \
      SSM_THROW(SSM_INTERNAL_ERROR);                                           \
  while (0)

/** @brief The time of the next event in the event queue.
 *
 *  Used by platform code to determine whether and when to invoke ssm_tick().
 *
 *  @returns the next event time, or #SSM_NEVER if the event queue is empty.
 */
ssm_time_t ssm_next_event_time(void);

/** @brief Reset the scheduler.
 *
 *  Set #now to 0; clear the event and activation record queues.
 *
 *  This does not need to be called before calling ssm_tick() for the first
 *  time; the global state automatically starts initialized.
 */
void ssm_reset(void);

/** @brief Advance the current model time.
 *
 *  Exposed internally so that platform code can perform external variable
 *  updates, to implement external inputs.
 *
 *  @a next must be later than ssm_now(), and earlier than or equal to
 *  ssm_next_event_time().
 *
 *  @param next the time to advance to.
 *
 *  @throws SSM_INTERNAL_ERROR @a next is earlier than or equal to ssm_now(), or
 *                            later than the earliest event in the event queue.
 */
void ssm_set_now(ssm_time_t next);

/** @brief Perform a (delayed) update on a variable.
 *
 *  Schedules all routine activation records sensitive to @a sv in the
 *  activation queue.
 *
 *  Exposed so that platform code can perform external variable updates.
 *
 *  Should only be called if the variable is scheduled to be updated #now.
 *
 *  @param sv the variable.
 *
 *  @throws SSM_INTERNAL_ERROR @a sv not ready to be updated #now.
 */
void ssm_update(ssm_sv_t *sv);

/** @brief Run the system for the next scheduled instant.
 *
 *  If there is nothing left to run in the current instant, advance #now to the
 *  time of the earliest event in the queue, if any.
 *
 *  Removes every event at the head of the event queue scheduled for #now,
 *  updates each variable's current value from its later value, and schedules
 *  all sensitive triggers in the activation queue. Finally, execute the step
 *  functions of all scheduled routines, in order of priority.
 *
 *  Should only be called if there are activation records to execute #now,
 *  or if #now is earlier than ssm_next_event_time().
 *
 *  @throws SSM_INTERNAL_ERROR not ready to tick in the current instant.
 */
void ssm_tick(void);

#endif /* _SSM_SCHED_H */
