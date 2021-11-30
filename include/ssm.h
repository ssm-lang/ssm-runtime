#ifndef _SSM_H
#define _SSM_H

/** \mainpage A Runtime Library for the Sparse Synchronous Model

\section intro_sec Introduction

The operation of this library was first described in

> Stephen A. Edwards and John Hui.
> The Sparse Synchronous Model.
> In Forum on Specification and Design Languages (FDL),
> Kiel, Germany, September 2020.
> http://www.cs.columbia.edu/~sedwards/papers/edwards2020sparse.pdf

\section usage_sec Usage

See [the detailed documentation](@ref all)

\section example Example


    #include "ssm.h"

    typedef struct {
      ssm_act_t act;
      ssm_event_t second;
    } main_act_t;

    ssm_stepf_t main_step;

    main_act_t *enter_main(struct ssm_act *parent,
                           ssm_priority_t priority,
                           ssm_depth_t depth) {
      ssm_act_t *act = ssm_enter(sizeof(main_act_t),
                                 main_step, parent, priority, depth);
      main_act_t *cont = container_of(act, main_act_t, act);
      ssm_initialize_event(&act->second);
    }

*/

#include <stdint.h>   /* For uint16_t, UINT64_MAX etc. */
#include <stdbool.h>  /* For bool, true, false */
#include <stdlib.h>   /* For size_t */
#include <assert.h>
#include <stddef.h>   /* For offsetof */

/** \defgroup all The SSM Runtime
 * \addtogroup all
 * @{
 */

#ifndef SSM_ACT_MALLOC
/** Allocation function for activation records.
 *
 * Given a number of bytes, return a void pointer to the base
 * of newly allocated space or 0 if no additional space is available */
#define SSM_ACT_MALLOC(size) malloc(size)
#endif

#ifndef SSM_ACT_FREE
/** Free function for activation records.
 *
 * The first argument is a pointer to the base of the activation record
 * being freed; the second is the size in bytes of the record being freed.
 */
#define SSM_ACT_FREE(ptr, size) free(ptr)
#endif

/** Underlying exception handler; must be overridden by each platform
 *
 * ssm_throw is declared as a weak symbol, meaning it will be left a null
 * pointer if the linker does not find a definition for this symbol in any
 * object file.
 */
void ssm_throw(int reason, const char *file, int line, const char *func);

/** Invoked when a process must terminate, e.g., when memory or queue space is
 * exhausted. Not expected to return.
 *
 * Argument passed is an ssm_error_t indicating why the failure occurred.
 * Default behavior is to exit with reason as the exit code, but can be
 * overridden by defining ssm_throw.
 */
#define SSM_THROW(reason) ssm_throw(reason, __FILE__, __LINE__, __func__)

/** Error codes, indicating reason for failure.
 *
 * Platforms may extend the list of errors using SSM_PLATFORM_ERROR like this:
 *
 *      enum {
 *        SSM_CUSTOM_ERROR1 = SSM_PLATFORM_ERROR,
 *        // etc.
 *      };
 */
enum ssm_error_t {
  /** Reserved for unforeseen, non-user-facing errors. */
  SSM_INTERNAL_ERROR = 1,
  /** Tried to insert into full activation record queue. */
  SSM_EXHAUSTED_ACT_QUEUE,
  /** Tried to insert into full event queue. */
  SSM_EXHAUSTED_EVENT_QUEUE,
  /** Could not allocate more memory. */
  SSM_EXHAUSTED_MEMORY,
  /** Tried to exceed available recursion depth. */
  SSM_EXHAUSTED_PRIORITY,
  /** Invalid time, e.g., scheduled delayed assignment at an earlier time. */
  SSM_INVALID_TIME,
  /** Start of platform-specific error code range. */
  SSM_PLATFORM_ERROR
};

/** Ticks per nanosecond */
#define SSM_NANOSECOND 1L
/** Ticks per microsecond */
#define SSM_MICROSECOND (SSM_NANOSECOND * 1000L)
/** Ticks per millisecond */
#define SSM_MILLISECOND (SSM_MICROSECOND * 1000L)
/** Ticks per second */
#define SSM_SECOND (SSM_MILLISECOND * 1000L)
/** Ticks per minute */
#define SSM_MINUTE (SSM_SECOND * 60L)
/** Ticks per hour */
#define SSM_HOUR (SSM_MINUTE * 60L)

/** Absolute time; never to overflow. */
typedef uint64_t ssm_time_t;

/** Time indicating something will never happen
 *
 * The value of this must be derived from the type of ssm_time_t
 */
#define SSM_NEVER UINT64_MAX

/** Thread priority.
 *
 *  Lower numbers execute first in an instant
 */
typedef uint32_t ssm_priority_t;

/** The priority for the entry point of an SSM program. */
#define SSM_ROOT_PRIORITY 0

/** Index of least significant bit in a group of priorities
 *
 * This only needs to represent the number of bits in the ssm_priority_t type.
 */
typedef uint8_t ssm_depth_t;

/** The depth at the entry point of an SSM program. */
#define SSM_ROOT_DEPTH (sizeof(ssm_priority_t) * 8)

struct ssm_sv;
struct ssm_trigger;
struct ssm_act;

/** The function that does an instant's work for a routine */
typedef void ssm_stepf_t(struct ssm_act *);

/** Activation record for an SSM routine

    Routine activation record "base class." A struct for a particular
    routine must start with this type but then may be followed by
    routine-specific fields.
*/
typedef struct ssm_act {
  ssm_stepf_t *step;       /**< C function for running this continuation */
  struct ssm_act *caller;  /**< Activation record of caller */
  uint16_t pc;             /**< Stored "program counter" for the function */
  uint16_t children;       /**< Number of running child threads */
  ssm_priority_t priority; /**< Execution priority; lower goes first */
  ssm_depth_t depth;       /**< Index of the LSB in our priority */
  bool scheduled;          /**< True when in the schedule queue */
} ssm_act_t;

/**  Indicates a routine should run when a scheduled variable is written
 *
 * Node in linked list of activation records, maintained by each scheduled
 * variable to determine which continuations should be scheduled when the
 * variable is updated.
 */
typedef struct ssm_trigger {
  struct ssm_trigger *next;      /**< Next sensitive trigger, if any */
  struct ssm_trigger **prev_ptr; /**< Pointer to ourself in previous list element */
  ssm_act_t *act;           /**< Routine triggered by this channel variable */
} ssm_trigger_t;


/**  SSM values are the size of a machine word. */
#if UINTPTR_MAX == 0xffffffffu
  typedef uint32_t ssm_word_t;
#elif UINTPTR_MAX == 0xffffffffffffffffu
  typedef uint64_t ssm_word_t;
#else
  #error Unsupported pointer size
#endif

struct ssm_object;

/** SSM values are either "packed" values or heap-allocated */
typedef union {
  struct ssm_object *heap_obj;    /**< Pointer to a heap-allocated object */
  ssm_word_t packed_obj;          /**< Packed value */
} ssm_value_t;

/**  The metadata accompanying any heap-allocated object
 *
 * When val_count is 0, tag is to be interpreted as an enum ssm_builtin
 */
struct ssm_mm_header {
  uint8_t val_count;      /**< Size of this object's payload */
  uint8_t tag;            /**< Which variant is inhabited by this object */
  uint8_t ref_count;      /**< The number of references to this object */
};

/**  Built-in types that cannot be expressed as a product of words */
enum ssm_builtin {
  SSM_TIMESTAMP_T,        /**< 64-bit timestamps */
  SSM_SV_T,               /**< Scheduled variables */
};

/**  Heap-allocated SSM object, with memory management metadata header
 *
 * The payload array is declared of size 1 here, but is of varying size in
 * practice. The actual size should be looked up in the header.
 */
struct ssm_object {
  struct ssm_mm_header mm;    /**< Memory management metadata */
  ssm_value_t payload[1];     /**< Heap object payload */
};

/** A variable that may have scheduled updates and triggers
 *
 * This is the "base class" for other scheduled variable types.
 *
 * On its own, this represents a pure event variable, i.e., a
 * scheduled variable with no data/payload.  The presence of an event on
 * such a variable can be tested with ssm_event_on() as well as awaited
 * with triggers.
 *
 * The update field must point to code that copies the new value of
 * the scheduled variable into its current value.  For pure events,
 * this function may do nothing, but the pointer must be non-zero.
 *
 * This can also be embedded in a wrapper struct/class to implement a scheduled
 * variable with a payload. In this case, the payload should also be embedded
 * in that wrapper class, and the vtable should have update/assign/later
 * methods specialized to be aware of the size and layout of the wrapper class.
 *
 * An invariant:
 * `later_time` != #SSM_NEVER if and only if this variable in the event queue.
 */
typedef struct ssm_sv {
  struct ssm_mm_header mm;     /**< Memory management metadata */
  ssm_time_t later_time;       /**< When the variable should be next updated */
  ssm_time_t last_updated;     /**< When the variable was last updated */
  ssm_trigger_t *triggers;     /**< List of sensitive continuations */
  ssm_value_t value;
  ssm_value_t later_value;
} ssm_sv_t;

/** Construct an ssm_value_t out of a raw integral 31-bit value */
#define ssm_marshal(v) (ssm_value_t) {.packed_obj = ((v) << 1 | 1)}

/** Extract an integral value out of a packed ssm_value_t */
#define ssm_unmarshal(v) ((v).packed_obj >> 1)

/** Indicate writing to a variable should trigger a routine
 *
 * Add a trigger to a variable's list of triggers.
 * When the scheduled variable is written, the scheduler
 * will run the trigger's routine routine.
 *
 * If a routine calls ssm_sensitize() on a trigger, it must call
 * ssm_desensitize() on the trigger if it ever calls ssm_leave() to
 * ensure a terminated routine is never inadvertantly triggered.
 */
extern void ssm_sensitize(ssm_sv_t *, ssm_trigger_t *);

/** Disable a sensitized routine
 *
 * Remove the trigger from its variable.  Only call this on
 * a previously-sensitized trigger.
 */
extern void ssm_desensitize(ssm_trigger_t *);

/** Schedule a routine to run in the current instant
 *
 * Enter the given activation record into the queue of activation
 * records.  This is idempotent: it may be called multiple times on
 * the same activation record within an instant; only the first call
 * has any effect.
 *
 * Invokes #SSM_RESOURCES_EXHAUSTED("ssm_activate") if the activation record queue is full.
 */
extern void ssm_activate(ssm_act_t *);

/** Execute a routine immediately */
#define ssm_call(act) (act)->step((act))

/** Initialize the activation record of a routine before entering */
#define ssm_enter(act, stepf, parent, priority, depth) \
  ++parent->children,\
  *act = (ssm_act_t) {\
      .step = stepf,\
      .caller = parent,\
      .pc = 0,\
      .children = 0,\
      .priority = priority,\
      .depth = depth,\
      .scheduled = false,\
  }

/** Destruct the activation record of a routine before leaving
 *
 * Returns pointer to caller if it was the last child
 */
#define ssm_leave(act) \
  --(act)->caller->children == 0 ? (act)->caller : (ssm_act_t *) NULL

/** Return true if there is an event on the given variable in the current instant
 */
bool ssm_event_on(ssm_sv_t *var /**< Variable: must be non-NULL */ );

/** Initialize a scheduled variable
 *
 * Call this to initialize the contents of a newly allocated scheduled
 * variable, e.g., after ssm_enter()
 *
 * Leaves the values of the variable uninitialized.
 */
void ssm_initialize(ssm_sv_t *var);

/** Instantaneous assignment to a scheduled variable
 *
 * Call this to assign the specified value to the variable.
 *
 * Wakes up all sensitive processes at a lower priority.
 *
 * Does not overwrite scheduled assignment.
 */
void ssm_assign(ssm_sv_t *var, ssm_priority_t prio, ssm_value_t value);


/** Delayed assignment to a scheduled variable.
 *
 * Call this to schedule a delayed assignment at a later time, which must be
 * strictly greater than #now.
 */
void ssm_later(ssm_sv_t *var, ssm_time_t later, ssm_value_t value);

/** Perform a (delayed) update on a variable, scheduling all sensitive triggers
 *
 * This is exposed so that platform code can perform external variable updates,
 * and should not be called by user code.
 */
void ssm_update(ssm_sv_t *sv);

/** Schedule a future update to a variable
 *
 * Add an event to the global event queue for the given variable,
 * replacing any pending event.
 */
void ssm_schedule(ssm_sv_t *var, /**< Variable to schedule: non-NULL */
		  ssm_time_t later
		  /**< Event time; must be in the future (greater than #now) */);

/** Unschedule any pending event on a variable
 *
 * If there is a pending event on the given variable, remove the event
 * from the queue.  Nothing happens if the variable does not have a
 * pending event.
 */
void ssm_unschedule(ssm_sv_t *var);

/** Activate routines triggered by a variable
 *
 * Call this when a scheduled variable is assigned in the current instant
 * (i.e., not scheduled)
 *
 * The given priority should be that of the routine doing the update.
 * Instantaneous assignment can only activate lower-priority (i.e., later)
 * routines in the same instant.
 */
void ssm_trigger(ssm_sv_t *var, /**< Variable being assigned */
		 ssm_priority_t priority
		 /**< Priority of the routine doing the assignment. */
		 );

/** Return the time of the next event in the queue or #SSM_NEVER
 *
 * Typically used by the platform code that ultimately invokes ssm_tick().
 */
ssm_time_t ssm_next_event_time(void);

/** Return the current model time */
ssm_time_t ssm_now(void);

/** Advance the current model time
 *
 * This is only exposed for platform code to perform external variable updates,
 * and should not be called by user code. It is the caller's responsibility to
 * ensure that next is earlier than the earliest event time in the queue.
 */
void ssm_set_now(ssm_time_t next);

/** Run the system for the next scheduled instant
 *
 * Typically run by the platform code, not the SSM program per se.
 *
 * If there is nothing left to run in the current instant, advance #now to the
 * time of the earliest event in the queue, if any.
 *
 * Remove every event at the head of the event queue scheduled for
 * #now, update the variable's current value by calling its
 * (type-specific) update function, and schedule all the triggers in
 * the activation queue.
 *
 * Remove the activation record in the activation record queue with the
 * lowest priority number and execute its "step" function.
 */
void ssm_tick();

/** Reset the scheduler.
 *
 *  Set now to 0; clear the event and activation record queues.  This
 *  does not need to be called before calling ssm_tick() for the first
 *  time; the global state automatically starts initialized.
 */
void ssm_reset();

/** An activation record for the parent of the topmost routine
 *
 * When you are starting up your SSM system, pass a pointer to this as
 * the parent of your topmost function.  E.g., if `main` is your topmost
 * function and your `enter_main()` function takes its parent as the first
 * argument,
 *
 * ~~~{.c}
 * main_act_t *enter_main(ssm_act_t *, ssm_priority_t, ssm_depth_t);
 * void step_main(ssm_act_t *act);
 *
 * enter_main(&ssm_top_parent, SSM_ROOT_PRIORITY, SSM_ROOT_DEPTH)
 * ~~~
 *
 * Here, `enter_main()` should cause ssm_enter() to be called with
 *
 * ~~~{.c}
 * ssm_enter(sizeof(main_act_t), step_main, &ssm_top_parent, SSM_ROOT_PRIORITY, SSM_ROOT_DEPTH)
 * ~~~
 */
extern ssm_act_t ssm_top_parent;


/**
 * Implementation of container_of that falls back to ISO C99 when GNU C is not
 * available (from https://stackoverflow.com/a/10269925/10497710)
 */
#ifdef __GNUC__
#define member_type(type, member) __typeof__(((type *)0)->member)
#else
#define member_type(type, member) const void
#endif
#define container_of(ptr, type, member)                                        \
  ((type *)((char *)(member_type(type, member) *){ptr} -                       \
            offsetof(type, member)))

/** @} */

#endif
