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
  SSM_NOT_READY,
  /**< Not yet ready to perform the requested action. */
  SSM_INVALID_TIME,
  /**< Specified invalid time, e.g., scheduled assignment at an earlier time. */
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

struct ssm_mm;
struct ssm_sv;
struct ssm_trigger;
struct ssm_act;

/**
 * @addtogroup mem
 * @{
 */

/** @brief Values are 32-bits, the largest supported machine word size. */
typedef uint32_t ssm_word_t;

/** @brief The metadata accompanying any heap-allocated object.
 *
 *  Should be embedded in heap-allocated objects as the first field (at memory
 *  offset 0).
 *
 *  The @a ref_count is used by ssm_new(), ssm_dup(), ssm_drop(), and
 *  ssm_reuse() for reference counting, and should always be greater than 1 for
 *  live objects.
 *
 *  In any heap-allocated object, #ssm_value_t fields that point to other
 *  heap-allocated appear last and be packed contiguously. The @a val_count
 *  indicates how many of them exist in the payload, and @a val_offset indicates
 *  the number of bytes past the beginning of the memory header at which the
 *  first #ssm_value_t may be found.
 *
 *  @a tag values greater than #SSM_BUILTIN_BASE indicate that the payload
 *  inhabits a builtin type.
 */
struct ssm_mm {
  uint8_t ref_count;  /**< The number of references to this object. */
  uint8_t tag;        /**< Which variant is inhabited by this object. */
  uint8_t val_count;  /**< Number of #ssm_value_t values in payload. */
  uint8_t val_offset; /**< Number of bytes at which values can be found. */
};

/** @brief SSM values are either "packed" values or heap-allocated. */
typedef union {
  struct ssm_mm *heap_ptr; /**< Pointer to a heap-allocated object. */
  ssm_word_t packed_val;   /**< Packed value. */
} ssm_value_t;

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

#define SSM_NIL                                                                \
  (ssm_value_t) { .packed_val = 0xffffffff }

/** @} */

/**
 * @addtogroup time
 * @{
 */

/** @brief Absolute time; never to overflow. */
typedef uint64_t ssm_time_t;

/** @brief Heap-allocated time values.
 *
 *  @invariant for all `struct ssm_time t`, `ssm_mm_is(SSM_TIME_T, &t.mm)`.
 */
struct ssm_time {
  struct ssm_mm mm; /**< Embedded memory management header. */
  ssm_time_t time;  /**< Time value payload. */
};

/** @brief Time indicating something will never happen.
 *
 *  The value of this must be derived from the underlying type of #ssm_time_t.
 */
#define SSM_NEVER UINT64_MAX

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
 *  Scheduled variables are heap-allocated built-in types that represent
 *  variables with reference-like semantics in SSM. This struct only represents
 *  the data specific to scheduled variables, and is not meant to be declared on
 *  its own. Instead, it should be allocated as part of an #ssm_mm using
 *  ssm_new().
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
 *  @invariant for all `ssm_sv_t v`, `ssm_mm_is(SSM_SV_T, &v.mm)`.
 */
typedef struct ssm_sv {
  struct ssm_mm mm;
  ssm_time_t later_time;   /**< When the variable should be next updated. */
  ssm_time_t last_updated; /**< When the variable was last updated. */
  ssm_trigger_t *triggers; /**< List of sensitive continuations. */
  ssm_value_t value;       /**< Current value. */
  ssm_value_t later_value; /**< Buffered value for delayed assignment. */
} ssm_sv_t;

/** @brief Initial value of an #ssm_sv.
 *
 *  For example:
 *
 *      ssm_value_t sv = ssm_new_builtin(SSM_SV_T);
 *      ssm_sv_init(sv, ssm_marshal(0));
 *
 *  @note Initialization does not count as an update event, so @a last_updated
 *        is initialized to #SSM_NEVER.
 *  @note The initial value of @a later_value is left unspecified.
 *
 *  @param sv   pointer to the scheduled variable.
 *  @param val  the initial value of the schedueld variable.
 */
#define ssm_sv_init(sv, val)                                                   \
  do {                                                                         \
    ssm_to_sv(sv)->later_time = SSM_NEVER;                                     \
    ssm_to_sv(sv)->last_updated = SSM_NEVER;                                   \
    ssm_to_sv(sv)->triggers = NULL;                                            \
    ssm_to_sv(sv)->value = val;                                                \
    ssm_to_sv(sv)->later_value = SSM_NIL;                                      \
  } while (0)

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

/** @} */

/**
 * @addtogroup mem
 * @{
 */

/** @brief Obtain pointer to #ssm_value_t in heap object at the given index.
 *
 *  @note This macro is unsafe in the sense that it does not check whether the
 *  given @a vo is consistent to the @a val_offset in @a mp, nor does it check
 *  that the given @a vi is within the bounds of the @a val_count in @a mp.
 *  However, it only performs pure pointer arithmetic, and will not attempt to
 *  access any memory itself.
 *
 *  @param mp pointer to the #ssm_mm header of the object.
 *  @param vo val_offset of
 *  @param vi index of the #ssm_value_t
 *  @returns  pointer to the #ssm_value_t at index @a vi under @a mp.
 */
#define SSM_OBJ_VAL(mp, vo, vi) (&((ssm_value_t *)((uint8_t *)(mp) + (vo)))[vi])

/** @brief Compute the size of heap object, given its @a val parameters.
 *
 *  @param vc the number of #ssm_value_t in the object, i.e., the @a val_count.
 *  @param vo the @a val_offset of the object, in bytes.
 *  @returns  the size of the heap object.
 */
#define SSM_OBJ_SIZE(vc, vo) ((vo) + (sizeof(ssm_value_t) * (vc)))

/** @brief Compute the size of a heap object pointed to by an #ssm_value_t.
 *
 *  @note This is unsafe in that it does not check whether @a v is a valid heap
 *  pointer. The behavior is only defined when `ssm_on_heap(v)`.
 *
 *  @param v  an #ssm_value_t that points to a heap object.
 *  @returns  the size of the heap object pointed to by @a v.
 */
#define ssm_obj_size(v)                                                        \
  SSM_OBJ_SIZE((v).heap_ptr->val_count, (v).heap_ptr->val_offset)

/** @brief Whether two sizes are compatible, for ssm_reuse().
 *
 *  @param a  the size of the object to be freed.
 *  @param b  the size of the object to be allocated.
 *  @returns  whether the heap space for @a a can be used for allocating @a b.
 */
#define SSM_SIZE_COMPATIBLE(a, b) ((a) == (b))

#define SSM_ADT_SIZE(vc)                                                       \
  sizeof(struct {                                                              \
    struct ssm_mm mm;                                                          \
    ssm_value_t payload[vc];                                                   \
  })

#define SSM_ADT_VAL_OFFSET                                                     \
  offsetof(                                                                    \
      struct {                                                                 \
        struct ssm_mm mm;                                                      \
        ssm_value_t payload[1];                                                \
      },                                                                       \
      payload)

#define ssm_adt_val(v, i) (*ssm_obj_val((v).heap_ptr, SSM_ADT_VAL_OFFSET, i))

/** @brief Tags equal to and greater than this value are builtin types. */
#define SSM_BUILTIN_BASE (1u << 7)

/** @brief Whether an mm header is embedded in a builtin-type object.
 *
 *  It makes no sense to allocate zero-size objects, so when @a val_count is
 *  zero, it indicates that it is a builtin object.
 *
 *  @param mp pointer to the #ssm_mm.
 *  @returns  non-zero if it is builtin, zero otherwise.
 */
#define ssm_is_builtin(tg) ((tg) >= SSM_BUILTIN_BASE)

/** @brief Built-in types that are stored in the heap.
 *
 *  Type enumerated here are chosen because they cannot be easily or efficiently
 *  expressed as a product of words. For instance, 64-bit timestamps cannot be
 *  directly stored in the payload of a regular heap object, where even-numbered
 *  timestamps may be misinterpreted as pointers.
 */
enum ssm_builtin {
  SSM_TIME_T = SSM_BUILTIN_BASE + 1, /**< 64-bit timestamps, #ssm_time_t */
  SSM_SV_T,                          /**< Scheduled variables, #ssm_sv_t */
};

#define SSM_BUILTIN_SIZE(tg)                                                   \
  ((size_t[]){                                                                 \
      [SSM_TIME_T - SSM_BUILTIN_BASE] = sizeof(struct ssm_time),               \
      [SSM_SV_T - SSM_BUILTIN_BASE] = sizeof(ssm_sv_t),                        \
  }[(tg)-SSM_BUILTIN_BASE])

#define SSM_BUILTIN_VAL_COUNT(tg)                                              \
  ((size_t[]){                                                                 \
      [SSM_TIME_T - SSM_BUILTIN_BASE] = 0,                                     \
      [SSM_SV_T - SSM_BUILTIN_BASE] = 2,                                       \
  }[(tg)-SSM_BUILTIN_BASE])

/** @brief Lookup the size of a builtin.
 *
 *  @param b  an #ssm_builtin indicating the type.
 *  @returns  the size of a builtin of type @a b, in bytes.
 */
#define SSM_BUILTIN_VAL_OFFSET(tg)                                             \
  ((size_t[]){                                                                 \
      [SSM_TIME_T - SSM_BUILTIN_BASE] = sizeof(struct ssm_time),               \
      [SSM_SV_T - SSM_BUILTIN_BASE] = offsetof(ssm_sv_t, value),               \
  }[(tg)-SSM_BUILTIN_BASE])

#define ssm_new(tg, vc)                                                        \
  ssm_is_builtin(tg) ? SSM_NIL : ssm_new_unsafe(tg, vc, SSM_ADT_VAL_OFFSET)

#define ssm_new_builtin(tg)                                                    \
  ssm_is_builtin(tg) ? ssm_new_unsafe((tg), SSM_BUILTIN_VAL_COUNT(tg),         \
                                      SSM_BUILTIN_VAL_OFFSET(tg))              \
                     : SSM_NIL

/** @brief Duplicate a possible heap reference, incrementing its ref count.
 *
 *  If the caller knows @a v is definitely on the heap, call ssm_drop_unsafe()
 *  to eliminate the heap check (and omit call if it is definitely not on the
 *  heap).
 *
 *  @param v  pointer to the #ssm_mm header of the heap item.
 */
#define ssm_dup(v)                                                             \
  if (ssm_on_heap(v))                                                          \
  ssm_dup_unsafe(v)

/** @brief Drop a reference to a possible heap item, and free it if necessary.
 *
 *  If @a v is freed, all references held by the heap item itself will also be
 *  be dropped.
 *
 *  If the caller knows that @a v is definitely a heap item, call
 *  ssm_drop_unsafe() to eliminate the heap check (and omit call if it is
 *  definitely not on the heap).
 *
 *  @param v  #ssm_value_t to be dropped.
 */
#define ssm_drop(v)                                                            \
  if (ssm_on_heap(v))                                                          \
  ssm_drop_unsafe(v)

/** @brief Try to reuse a potentially heap-allocated value.
 *
 *  Performs a runtime check to ensure that @a v actually points to a heap item
 *  of sufficient size.
 *
 *  Calls ssm_new() to allocate an entirely new heap item if @a v cannot be
 *  reused.
 *
 *  If the caller is sure that @a v can definitely be reused to allocate a heap
 *  object with @a vc and @a tg, call ssm_reuse_unsafe() directly.
 *
 *  @param v    the value to try to reuse.
 *  @param vc   the number of #ssm_value_t values to be stored in the
 *              object payload.
 *  @param tg   the tag to initialize the @a mm header with.
 *  @returns    value pointing to the newly allocated object.
 */
#define ssm_reuse(v, tg, vc)                                                   \
  (ssm_on_heap(v) && SSM_SIZE_COMPATIBLE(ssm_obj_size(v), SSM_ADT_SIZE(vc))    \
       ? ssm_reuse_unsafe(v, tg, vc, SSM_ADT_VAL_OFFSET)                       \
       : ssm_new(tg, vc))

#define ssm_reuse_builtin(v, tg)                                               \
  (ssm_on_heap(v) &&                                                           \
           SSM_SIZE_COMPATIBLE(ssm_obj_size(v), SSM_BUILTIN_SIZE(tg))          \
       ? ssm_reuse_unsafe(v, tg, SSM_BUILTIN_VAL_COUNT(tg),                    \
                          SSM_BUILTIN_VAL_OFFSET(tg))                          \
       : ssm_new_builtin(tg))

/** @brief Allocate a new heap object to store word-size values.
 *
 *  When @a val_count is equal to #SSM_BUILTIN, @a tag is interpreted as an
 *  #ssm_builtin enumeration, and a memory block of that size is allocated.
 *
 *  The @a mm header in the returned object is initialized, but the
 *  @a data payload is not.
 *
 *  @param tag        the tag to initialize the @a mm header with.
 *  @param val_count  the number of #ssm_value_t to be stored in the payload.
 *  @param val_offset the byte offset at which #ssm_value_t are stored.
 *  @returns          value pointing to the newly allocated object.
 *
 *  @sa SSM_BUILTIN_SIZE().
 */
ssm_value_t ssm_new_unsafe(uint8_t tag, uint8_t val_count, uint8_t val_offset);

/** @brief Duplicate a heap reference, incrementing its ref count.
 *
 *  Called by ssm_dup().
 *
 *  @note assumes that @a v is a heap pointer, i.e., `ssm_on_heap(v)`.
 *
 *  @param v  pointer to the #ssm_mm header of the heap item.
 */
void ssm_dup_unsafe(ssm_value_t v);

/** @brief Drop a reference to a heap item, and free it if necessary.
 *
 *  If @a v is freed, all references held by the heap item itself will also be
 *  be dropped.
 *
 *  Called by ssm_drop().
 *
 *  @note assumes that @a v is a heap pointer, i.e., `ssm_on_heap(v)`.
 *
 *  @param v  #ssm_value_t to be dropped.
 */
void ssm_drop_unsafe(ssm_value_t v);

/** @brief Try to reuse a heap-allocated value.
 *
 *  Calls ssm_new() to allocate an entirely new heap item if @a v cannot be
 *  reused.
 *
 *  Called by ssm_reuse().
 *
 *  @note assumes that @a v is a heap pointer to an object of sufficient size.
 *
 *  @param v          the value to try to reuse.
 *  @param val_count  the number of #ssm_value_t values to be stored in the
 *                    object payload.
 *  @param tag        the tag to initialize the @a mm header with.
 *  @returns          value pointing to the newly allocated object.
 */
ssm_value_t ssm_reuse_unsafe(ssm_value_t v, uint8_t tag, uint8_t val_count,
                             uint8_t val_offset);

/** @brief Retrieve the tag of an SSM heap object.
 *
 *  Behavior is implementation defined for built-in types.
 *
 *  @param v  the #ssm_value_t whose tag is being retrieved.
 *  @returns  the tag of @a v.
 */
#define ssm_tag(v) (ssm_on_heap(v) ? (v).heap_ptr->tag : ssm_unmarshal(v))

/** @brief Access the heap object payload pointed to by an #ssm_value_t.
 *
 *  Provided for convenience.
 *
 *  @note The behavior of using this macro on an #ssm_value_t that does not
 *  point to a scheduled variable is undefined.
 *
 *  @param v  the #ssm_value_t
 *  @returns  pointer to the beginning of the value payload in the heap.
 */
#define ssm_to_obj(v) (&*container_of((v).heap_ptr, ssm_obj_t(1), mm)->payload)

/** @brief Access the heap-allocated time pointed to by an #ssm_value_t.
 *
 *  Provided for convenience.
 *
 *  @note The behavior of using this macro on an #ssm_value_t that does not
 *  point to a scheduled variable is undefined.
 *
 *  @param v  the #ssm_value_t
 *  @returns  pointer to the #ssm_time_t in the heap.
 */
#define ssm_to_time(v) (&container_of((v).heap_ptr, ssm_time_t, mm)->time)

/** @brief Access the scheduled variable payload pointed to by an #ssm_value_t.
 *
 *  Provided for convenience.
 *
 *  @note The behavior of using this macro on an #ssm_value_t that does not
 *  point to a scheduled variable is undefined.
 *
 *  @param v  the #ssm_value_t
 *  @returns  pointer to the #ssm_sv_t in the heap.
 */
#define ssm_to_sv(v) container_of((v).heap_ptr, ssm_sv_t, mm)

/** @brief Obtain the value of a scheduled variable.
 *
 *  @param v  #ssm_value_t that points to a scheduled variable.
 *  @returns  the value that @a v points to.
 */
#define ssm_deref(v) (ssm_to_sv(v)->value)

/** @brief Allocate a contiguous range of memory.
 *
 *  Memory will be allocated from an appropriately sized memory pool, if one is
 *  available. Guaranteed to be aligned against the smallest power of 2 greater
 *  than @a size.
 *
 *  @param size   the requested memory range size, in bytes.
 *  @returns      pointer to the first byte of the allocate memory block.
 */
void *ssm_mem_alloc(size_t size);

/** @brief Preallocate memory pages to ensure capacity in memory pools.
 *
 *  Does nothing if no memory pool will fit a block of @a size.
 *
 *  @param size       size whose memory pool should be preallocaed pages.
 *  @param num_pages  number of pages to allocate.
 */
void ssm_mem_prealloc(size_t size, size_t num_pages);

/** @brief Deallocate memory allocated by ssm_mem_alloc().
 *
 *  The behavior of freeing memory not allocated by ssm_mem_alloc() is
 *  undefined.
 *
 *  @param m      pointer to the memory range allocated by ssm_mem_alloc().
 *  @param size   the size of the memory range allocated by ssm_mem_alloc().
 */
void ssm_mem_free(void *m, size_t size);

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
