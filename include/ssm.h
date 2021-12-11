/** @file ssm.h
 *  @brief Interface to the SSM runtime.
 *
 *  This file contains the public-facing interface of the SSM runtime.
 *
 *  @author Stephen Edwards (sedwards-lab)
 *  @author John Hui (j-hui)
 */
#ifndef _SSM_H
#define _SSM_H

#include <stdbool.h> /* For bool, true, false */
#include <stddef.h>  /* For offsetof */
#include <stdint.h>  /* For uint16_t, UINT64_MAX etc. */
#include <stdlib.h>  /* For size_t */

/**
 * @addtogroup error
 * @{
 */

/** @brief Error codes indicating reason for failure.
 *
 *  Platforms may extend the list of errors using #SSM_PLATFORM_ERROR like this:
 *
 *  ~~~{.c}
 *  enum {
 *    SSM_CUSTOM_ERROR_CODE1 = SSM_PLATFORM_ERROR,
 *    SSM_CUSTOM_ERROR_CODE2,
 *    // etc.
 *  };
 *  ~~~
 */
typedef enum ssm_error {
  SSM_INTERNAL_ERROR = 1,
  /**< Reserved for unforeseen, non-user-facing errors. */
  SSM_EXHAUSTED_ACT_QUEUE,
  /**< Tried to insert into full activation record queue. */
  SSM_EXHAUSTED_EVENT_QUEUE,
  /**< Tried to insert into full event queue. */
  SSM_EXHAUSTED_MEMORY,
  /**< Could not allocate more memory. */
  SSM_EXHAUSTED_PRIORITY,
  /**< Tried to exceed available recursion depth. */
  SSM_INVALID_TIME,
  /**< Invalid time, e.g., scheduled delayed assignment at an earlier time. */
  SSM_INVALID_MEMORY,
  /**< Invalid memory layout, e.g., using a pointer where int was expected. */
  SSM_PLATFORM_ERROR
  /**< Start of platform-specific error code range. */
} ssm_error_t;

/** @brief Terminate due to a non-recoverable error, with a specified reason.
 *
 *  Invoked when a process must terminate, e.g., when memory or queue space is
 *  exhausted. Not expected to terminate.
 *
 *  Wraps the underlying ssm_throw() exception handler, passing along the
 *  file name, line number, and function name in the source file where the error
 *  was thrown.
 *
 *  @param reason an #ssm_error_t specifying the reason for the error.
 */
#define SSM_THROW(reason) ssm_throw(reason, __FILE__, __LINE__, __func__)

/** @brief Underlying exception handler; must be overridden by each platform.
 *
 *  This is left undefined by the SSM runtime for portability. On platforms
 *  where exit() is available, that may be used. Where possible, an exception
 *  handler may also log additional information using the given parameters for
 *  better debuggability.
 *
 *  @param reason an #ssm_error_t specifying the reason for the error.
 *  @param file   the file name of the source file where the error was thrown.
 *  @param line   the line number of the source file where the error was thrown.
 *  @param func   the function name where the error was thrown.
 */
void ssm_throw(ssm_error_t reason, const char *file, int line,
               const char *func);

/** @} */

struct ssm_sv;
struct ssm_trigger;
struct ssm_act;
struct ssm_object;
struct ssm_time;

/**
 * @addtogroup mem
 * @{
 */

/** @brief SSM values are the size of a machine word. */
#if UINTPTR_MAX == 0xffffffffu
typedef uint32_t ssm_word_t;
#elif UINTPTR_MAX == 0xffffffffffffffffu
typedef uint64_t ssm_word_t;
#else
#error Unsupported pointer size
#endif

/** @brief The metadata accompanying any heap-allocated object.
 *
 *  When @a val_count is 0, @a tag is to be interpreted as an #ssm_builtin.
 */
struct ssm_mm {
  uint8_t val_count; /**< Number of ssm_value_t values in payload. */
  uint8_t tag;       /**< Which variant is inhabited by this object. */
  uint8_t ref_count; /**< The number of references to this object. */
};

/** @brief Magic @a val_count value indicating a heap object is a builtin. */
#define SSM_BUILTIN 0

/** @brief Built-in types that are stored in the heap.
 *
 *  Type enumerated here are chosen because they cannot be easily or efficiently
 *  expressed as a product of words. For instance, 64-bit timestamps cannot be
 *  directly stored in the payload of an #ssm_object, where even-numbered
 *  timestamps may be misinterpreted as pointers.
 */
enum ssm_builtin {
  SSM_TIME_T = 1, /**< 64-bit timestamps, #ssm_time_t */
  SSM_SV_T,       /**< Scheduled variables, #ssm_sv_t */
};

/** @brief SSM values are either "packed" values or heap-allocated. */
typedef union {
  struct ssm_mm *heap_ptr; /**< Pointer to a heap-allocated object. */
  ssm_word_t packed_val;   /**< Packed value. */
} ssm_value_t;

/** @brief Heap-allocated SSM object, with memory management metadata header.
 *
 *  The payload array is declared of size 1 here, but is of varying size in
 *  practice. The actual size should be looked up in the @a mm header.
 */
struct ssm_object {
  struct ssm_mm mm;       /**< Memory management metadata. */
  ssm_value_t payload[1]; /**< Heap object payload. */
};

#define ssm_obj_payload_count(obj_t)                                           \
  (sizeof(((obj_t *)0)->payload) / sizeof(ssm_value_t))

/** @brief Construct an #ssm_value_t from a 31-bit integral value.
 *
 *  @param v  the 31-bit integral value.
 *  @return   a packed #ssm_value_t.
 */
#define ssm_marshal(v)                                                         \
  (ssm_value_t) { .packed_val = ((v) << 1 | 1) }

/** @brief Extract an integral value from a packed #ssm_value_t.
 *
 *  @param v  the packed #ssm_value_t.
 *  @return   the raw 31-bit integral value.
 */
#define ssm_unmarshal(v) ((v).packed_val >> 1)

/** @brief Whether a value is on the heap (i.e., is an object).
 *
 *  @param v  pointer to the #ssm_value_t.
 *  @returns  non-zero if on heap, zero otherwise.
 */
#define ssm_on_heap(v) (((v).packed_val & 0x1) == 0)

/** @brief Whether an mm header is embedded in a builtin-type object.
 *
 *  It makes no sense to allocate zero-size objects, so when @a val_count is
 *  zero, it indicates that it is a builtin object.
 *
 *  @param mp pointer to the #ssm_mm.
 *  @returns  non-zero if it is builtin, zero otherwise.
 */
#define ssm_mm_is_builtin(mp) ((mp)->val_count == SSM_BUILTIN)

/** @brief Whether an mm header is embedded in a particular builtin-type object.
 *
 *  @param bt #ssm_builtin type enumeration.
 *  @param mp pointer to the #ssm_mm
 *  @returns  non-zero if it @a mm is of type @a bt, zero otherwise.
 *
 *  @sa #ssm_builtin.
 */
#define ssm_mm_is(bt, mp) (ssm_mm_is_builtin(mp) && (mp)->tag == (bt))

/** @brief Convert an #ssm_value_t to an #ssm_object.
 *
 *  Provided for convenience; zero runtime cost.
 *
 *  @param v  the #ssm_value_t
 *  @returns  pointer to the #ssm_object in the heap.
 */
#define ssm_to_obj(v) (container_of((v).heap_ptr, struct ssm_object, mm))

/** @brief Convert an #ssm_object pointer to an #ssm_value_t.
 *
 *  Provided for convenience; zero runtime cost.
 *
 *  @param o  pointer to the #ssm_object.
 *  @returns  #ssm_value_t of the pointer to @a o.
 */
#define ssm_from_obj(o)                                                        \
  (ssm_value_t) { .heap_ptr = &(o)->mm }

/** @brief Allocate a new heap object to store word-size values.
 *
 *  @a val_count must be greater than 0, i.e., not equal to #SSM_BUILTIN.
 *
 *  The @a mm header in the returned #ssm_object is initialized, but the
 *  @a payload is not.
 *
 *  @param val_count  the number of #ssm_value_t values to be stored in the
 *                    object payload.
 *  @param tag        the tag to initialize the @a mm header with.
 *  @returns          pointer to a newly allocated and initialized #ssm_object.
 *
 *  @throws SSM_INTERNAL_ERROR  @a val_count was not greater than 0.
 *
 *  @invariant `!ssm_mm_builtin(&ssm_new(any)->mm)`.
 */
struct ssm_object *ssm_new(uint8_t val_count, uint8_t tag);

/** @todo document */
void ssm_dup(struct ssm_mm *mm);

/** @todo document */
void ssm_drop(struct ssm_mm *mm);

/** @todo document */
struct ssm_mm *ssm_reuse(struct ssm_mm *mm);

/** @brief Allocate memory.
 *
 *  Belongs to the memory allocator.
 *
 *  @TODO: document
 */
void *ssm_mem_alloc(size_t size);

/** @brief Deallocate memory.
 *
 *  @TODO: document
 */
void ssm_mem_free(void *mm, size_t size);

/** @} */

/**
 * @addtogroup time
 * @{
 */

/** @brief Absolute time; never to overflow. */
typedef uint64_t ssm_time_t;

/** @brief Time indicating something will never happen.
 *
 *  The value of this must be derived from the underlying type of #ssm_time_t.
 */
#define SSM_NEVER UINT64_MAX

/** @brief Time value object, stored in the heap.
 *
 *  @invariant For all `struct ssm_time t`, `ssm_mm_is_builtin(SSM_TIME_T, t)`.
 */
struct ssm_time {
  struct ssm_mm mm;
  ssm_time_t time;
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

/** @brief The current model time.
 *
 *  @returns the current model time.
 */
ssm_time_t ssm_now(void);

/** @brief Allocate a time object on the heap.
 *
 *  Use ssm_drop() on the @a mm header to free.
 *
 *  @param time value to initialize the time object with.
 *  @returns    pointer to an #ssm_time in the heap.
 *
 *  @throws SSM_EXHAUSTED_MEMORY out of memory.
 *
 *  @invariant `ssm_mm_is(SSM_TIME_T, &ssm_new_time(any)->mm)`.
 */
struct ssm_time *ssm_new_time(ssm_time_t time);

/** @brief Convert an #ssm_value_t to an #ssm_time.
 *
 *  Provided for convenience; zero runtime cost.
 *
 *  @param v  the #ssm_value_t
 *  @returns  pointer to the #ssm_time in the heap.
 */
#define ssm_to_time(v) (container_of((v).heap_ptr, struct ssm_time, mm))

/** @brief Convert an #ssm_time pointer to an #ssm_value_t.
 *
 *  Provided for convenience; zero runtime cost.
 *
 *  @param t  pointer to the #ssm_object.
 *  @returns  #ssm_value_t of the pointer to @a t.
 */
#define ssm_from_time(t)                                                       \
  (ssm_value_t) { .heap_ptr = &(t)->mm }

/** @} */

/**
 * @addtogroup act
 * @{
 */

/** @brief Thread priority.
 *
 *  Lower numbers execute first in an instant.
 */
typedef uint32_t ssm_priority_t;

/** @brief The priority for the entry point of an SSM program. */
#define SSM_ROOT_PRIORITY 0

/** @brief Index of least significant bit in a group of priorities
 *
 *  This only needs to represent the number of bits in the #ssm_priority_t type.
 */
typedef uint8_t ssm_depth_t;

/** @brief The depth at the entry point of an SSM program. */
#define SSM_ROOT_DEPTH (sizeof(ssm_priority_t) * 8)

/** @brief The function that does an instant's work for a routine. */
typedef void ssm_stepf_t(struct ssm_act *);

/** @brief Activation record for an SSM routine.
 *
 *  Routine activation record "base class." A struct for a particular
 *  routine must start with this type but then may be followed by
 *  routine-specific fields.
 */
typedef struct ssm_act {
  ssm_stepf_t *step;       /**< C function for running this continuation. */
  struct ssm_act *caller;  /**< Activation record of caller. */
  uint16_t pc;             /**< Stored "program counter" for the function. */
  uint16_t children;       /**< Number of running child threads. */
  ssm_priority_t priority; /**< Execution priority; lower goes first. */
  ssm_depth_t depth;       /**< Index of the LSB in our priority. */
  bool scheduled;          /**< True when in the schedule queue. */
} ssm_act_t;

/** @brief Indicates a routine should run when a scheduled variable is written.
 *
 *  Node in linked list of activation records, maintained by each scheduled
 *  variable to determine which continuations should be scheduled when the
 *  variable is updated.
 */
typedef struct ssm_trigger {
  struct ssm_trigger *next;      /**< Next sensitive trigger, if any. */
  struct ssm_trigger **prev_ptr; /**< Pointer to self in previous element. */
  struct ssm_act *act;           /**< Routine triggered by this variable. */
} ssm_trigger_t;

/** @brief Allocate and initialize a routine activation record.
 *
 *  Uses the underlying memory allocator to allocate an activation record of
 *  a given @a size, and initializes it with the rest of the fields. Also
 *  increments the number of children that @a parent has.
 *
 *  This function assumes that the embedded #ssm_act_t is at the beginning of
 *  the allocated activation record. That is, it has the following layout:
 *
 *  ~~~{.c}
 *  struct {
 *    ssm_act_t act;
 *    // Other fields
 *  };
 *  ~~~
 *
 *  @param size     the size of the routine activation record to allocate.
 *  @param step     the step function of the routine.
 *  @param parent   the parent (caller) of the activation record.
 *  @param priority the priority of the activation record.
 *  @param depth    the depth of the activation record.
 *  @returns        the newly initialized activation record.
 *
 *  @throws SSM_EXHAUSTED_MEMORY out of memory.
 */
ssm_act_t *ssm_enter(size_t size, ssm_stepf_t step, ssm_act_t *parent,
                     ssm_priority_t priority, ssm_depth_t depth);

/** @brief Destroy the activation record of a routine before leaving.
 *
 *  Calls the parent if @a act is the last child.
 *
 *  @param act  the activation record of the routine to leave.
 *  @param size the size of activation record.
 */
void ssm_leave(ssm_act_t *act, size_t size);

/** @brief Schedule a routine to run in the current instant.
 *
 *  This function is idempotent: it may be called multiple times on the same
 *  activation record within an instant; only the first call has any effect.
 *
 *  @param act activation record of the routine to schedule.
 *
 *  @throws SSM_EXHAUSTED_ACT_QUEUE the activation record is full.
 */
void ssm_activate(ssm_act_t *act);

/** @brief An activation record for the parent of the top-most routine.
 *
 *  This activation record should be used as the parent of the entry point.
 *  For example, if the entry point is named `main`, with activation record type
 *  `main_act_t` and step function `step_main`:
 *
 *  ~~~{.c}
 *  ssm_enter(sizeof(main_act_t), step_main, &ssm_top_parent,
 *            SSM_ROOT_PRIORITY, SSM_ROOT_DEPTH)
 *  ~~~
 */
extern ssm_act_t ssm_top_parent;

/** @} */

/**
 * @addtogroup sv
 * @{
 */

/** @brief A scheduled variable that supported scheduled updates with triggers.
 *
 *  Scheduled variables are heap-allocated structures that represent variables
 *  with reference-like semantics in SSM.
 *
 *  Routines may directly assign to them in the current instant (ssm_assign()),
 *  or schedule a delayed assignment to them (ssm_later()). The @a last_updated
 *  time is recorded in each case, sensitive routines, i.e., @a triggers, are
 *  woken up.
 *
 *  At most one delayed assignment may be scheduled at a time, but a single
 *  update may wake any number of sensitive routines.
 *
 *  @invariant @a later_time != #SSM_NEVER iff this variable in the event queue.
 *  @invariant For all `struct ssm_time t`, `ssm_mm_is_builtin(SSM_SV_T, t)`.
 */
typedef struct ssm_sv {
  struct ssm_mm mm;        /**< Memory management metadata. */
  ssm_time_t later_time;   /**< When the variable should be next updated. */
  ssm_time_t last_updated; /**< When the variable was last updated. */
  ssm_trigger_t *triggers; /**< List of sensitive continuations. */
  ssm_value_t value;       /**< Current value. */
  ssm_value_t later_value; /**< Buffered value for delayed assignment. */
} ssm_sv_t;

/** @brief Allocate and initialize a scheduled variable.
 *
 *  Initializes the @a value field with @a val, but leaves @a later_value
 *  uninitialized. Other field are initialized appropriately.
 *
 *  The scheduled variable is allocated on the heap.
 *
 *  Call ssm_drop() on the @a mm header to free.
 *
 *  @param val  value to initialize the sv value with.
 *  @returns    pointer to the allocated #ssm_sv_t object.
 *
 *  @throws SSM_EXHAUSTED_MEMORY out of memory.
 *
 *  @invariant `ssm_mm_is(SSM_SV_T, &ssm_new_sv(any)->mm)`.
 */
struct ssm_sv *ssm_new_sv(ssm_value_t val);

/** @brief Convert an #ssm_value_t to an #ssm_time.
 *
 *  Provided for convenience; zero runtime cost.
 *
 *  @param v  the #ssm_value_t
 *  @returns  pointer to the #ssm_time in the heap.
 */
#define ssm_to_sv(v) (container_of((v).heap_ptr, struct ssm_sv, mm))

/** @brief Convert an #ssm_time pointer to an #ssm_value_t.
 *
 *  Provided for convenience; zero runtime cost.
 *
 *  @param s  pointer to the #ssm_object.
 *  @returns  #ssm_value_t of the pointer to @a t.
 */
#define ssm_from_sv(s)                                                         \
  (ssm_value_t) { .heap_ptr = &(s)->mm }

/** @brief Whether there the variable was updated in the current instant.
 *
 *  @param v  #ssm_value_t that points to a scheduled variable.
 *  @returns  non-zero if the variable was updated in the current instant; zero
 *            otherwise.
 */
#define ssm_event_on(v) (ssm_to_sv(v)->last_updated == ssm_now())

/** @brief Obtain the value of a scheduled variable.
 *
 *  @param v  #ssm_value_t that points to a scheduled variable.
 *  @returns  the value that @a v points to.
 */
#define ssm_deref(v) (ssm_to_sv(v)->value)

/** @brief Obtain memory management header of a scheduled variable.
 *
 *  @param v  #ssm_value_t that points to a scheduled variable.
 *  @returns  a pointer to the #ssm_mm header of @a v.
 */
#define ssm_sv_mm(v) (&ssm_to_sv(v)->mm)

/** @brief Instantaneous assignment to a scheduled variable.
 *
 *  Updates the value of @a var in the current instant, and wakes up all
 *  sensitive processes at a lower priority than the calling routine.
 *
 *  Does not overwrite a scheduled assignment.
 *
 *  @param var    pointer to the scheduled variable.
 *  @param prio   priority of the calling routine.
 *  @param value  the value to be assigned to @a var.
 */
void ssm_assign(ssm_sv_t *var, ssm_priority_t prio, ssm_value_t value);

/** @brief Delayed assignment to a scheduled variable.
 *
 *  Schedules a delayed assignment to @a var at a later time.
 *
 *  Overwrites any previously scheduled update.
 *
 *  @param var    pointer to the scheduled variable.
 *  @param later  the time when the update should take place.
 *  @param value  the value to be assigned to @a var.
 *
 *  @throws SSM_INVALID_TIME          @a later is greater than ssm_now().
 *  @throws SSM_EXHAUSTED_EVENT_QUEUE event queue ran out of space.
 */
void ssm_later(ssm_sv_t *var, ssm_time_t later, ssm_value_t value);

/** @brief Sensitize a variable to a trigger.
 *
 *  Adds a trigger to a scheduled variable, so that @a trig's activation
 *  record is awoken when the variable is updated.
 *
 *  This function should be called by a routine before sleeping (yielding).
 *
 *  @param var  pointer to the scheduled variable.
 *  @param trig trigger to be registered on @a var.
 */
void ssm_sensitize(ssm_sv_t *var, ssm_trigger_t *trig);

/** @brief Desensitize a variable from a trigger.
 *
 *  Remove a trigger from its variable.
 *
 *  This function should be called by a routine after returning from sleeping.
 *
 *  @param trig the trigger.
 */
void ssm_desensitize(ssm_trigger_t *trig);

/** @brief Unschedule any pending events on a variable.
 *
 *  Should be called before the variable is dropped.
 *
 *  Nothing happens if the variable does not have a pending event.
 *
 *  @todo move this to ssm-internal, since this will just be part of drop.
 *
 *  @param var  the variable.
 */
void ssm_unschedule(ssm_sv_t *var);

/** @} */

/** @ingroup util
 *  @def member_type
 *  @brief Obtain the type of a struct member.
 *
 *  Intended for use in #container_of, for type safety.
 *
 *  When GNU C is not available, falls back to ISO C99 and returns `const void`
 *  (see https://stackoverflow.com/a/10269925/10497710).
 *
 *  For instance, given the following struct definition:
 *
 *  ~~~{.c}
 *  struct foo { int bar; float baz; };
 *  ~~~
 *
 *  `member_type(struct foo, baz)` expands to `float`.
 *
 *  @param type   the struct type.
 *  @param member the name of the member in @a type.
 *  @returns      the type of @a member in @a type.
 */
#ifdef __GNUC__
#define member_type(type, member) __typeof__(((type *)0)->member)
#else
#define member_type(type, member) const void
#endif

/** @ingroup util
 *  @brief Obtain the pointer to an outer, enclosing struct.
 *
 *  For example, with the following struct definition:
 *
 *  ~~~{.c}
 *  struct foo { int bar; float baz; };
 *  ~~~
 *
 *  Given some `float *p`, `container_of(p, struct foo, baz)` will return
 *  a pointer to the enclosing `struct foo`. The caller is responsible for
 *  ensuring that such an enclosing pointer actually exists.
 *
 *  Use this macro instead of pointer casting or pointer arithmetic; this macro
 *  is more principled in what it does and, with GNU C support, will trigger
 *  compiler warnings when @a ptr and the @a member type do not agree.
 *
 *  Adapted from the Linux kernel source
 *  (see https://stackoverflow.com/a/10269925/10497710).
 *
 *  @param ptr    pointer to member.
 *  @param type   type of enclosing struct.
 *  @param member name of member in enclosing struct.
 *  @returns      pointer to enclosing struct.
 */
#define container_of(ptr, type, member)                                        \
  ((type *)((char *)(member_type(type, member) *){ptr} -                       \
            offsetof(type, member)))

#endif
