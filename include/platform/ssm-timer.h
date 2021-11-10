/**
 * SSM 64-bit timer.
 *
 * Implemented using a 32-bit counter with an overflow handler that increments
 * the higher-order bits.
 *
 * When reading the time, readers must read the higher order bits both before
 * and after reading the lower bits from the counter, to account for counter
 * wraparound. If the two higher bit readings differ, then the most significant
 * bit of the lower bit reading is used to determine which higher bit reading to
 * use.
 *
 * This implementation's API supports performing the reading and calculation of
 * the 64-bit timestamp separately, to allow that calculation to be deferred
 * (e.g., performed outside of an interrupt handler).
 *
 * NOTE: does not support preemptible wraparound handler.
 */
#ifndef _PLATFORM_SSM_TIMER_H
#define _PLATFORM_SSM_TIMER_H

#include <stdint.h>   /* For uint16_t, UINT64_MAX etc. */

/** Number of bits used by lower-order bit counter. */
#define SSM_TIMER32_BITS 32

#ifndef PLATFORM_TIMER64
extern volatile uint32_t ssm_timer_hi;
/** Compute most significant bit of a lower-order bits. */
#define ssm_timer_lo_msb(lo) ((lo) & (0x1u << (SSM_TIMER32_BITS - 1)))
/** Shift higher-order 32-bit value to the top half of a 64-bit value. */
#define ssm_timer_hi_shift(hi) (((uint64_t)(hi)) << SSM_TIMER32_BITS)
/** Compute the 64-bit value from the higher-order and lower-order bits. */
#define ssm_timer_combine(hi, lo) (ssm_timer_hi_shift(hi) + (lo))

#define ssm_timer_read_to_from(p_hi0, p_lo, p_hi1, lo_from)                    \
  do {                                                                         \
    *(p_hi0) = ssm_timer_hi;                                                   \
    compiler_barrier();                                                        \
    *(p_lo) = lo_from;                                                         \
    compiler_barrier();                                                        \
    *(p_hi1) = ssm_timer_hi;                                                   \
  } while (0)

/**
 * Read higher-order bits, lower-order bits, and higher-order bits again, to
 * pointers p_hi0, p_lo, and p_hi1, respectively. Note that the caller is
 * responsible for allocating memory for p_hi0, p_lo, and p_hi1.
 */
#define ssm_timer_read_to(p_hi0, p_lo, p_hi1)                                  \
  ssm_timer_read_to_from(p_hi0, p_lo, p_hi1, ssm_timer32_read())

/** Compute the 64-bit timer from values read by ssm_timer_read_to. */
#define ssm_timer_calc(hi0, lo, hi1)                                           \
  ((hi0) == (hi1)         ? ssm_timer_combine(hi0, lo)                         \
   : ssm_timer_lo_msb(lo) ? ssm_timer_combine(hi0, lo)                         \
                          : ssm_timer_combine(hi1, lo))
#endif

#define SSM_TIMER32_TOTAL_BITS SSM_TIMER32_BITS
#define SSM_TIMER32_TOP                                                        \
  ((0x1u << (SSM_TIMER32_TOTAL_BITS - 1)) |                                    \
   ((0x1u << (SSM_TIMER32_TOTAL_BITS - 1)) - 1))
#define SSM_TIMER32_GUARD (SSM_TIMER32_TOP / 2)
#define SSM_TIMER32_OVERFLOW_HANDLE()                                          \
  do { extern volatile uint32_t ssm_timer_hi; ++ssm_timer_hi; } while (0)

typedef void (*ssm_timer_callback_t) (uint64_t time, void *user_data);
typedef void (*ssm_timer32_callback_t) (uint32_t time, void *user_data);

/**
 * For 32-bit timers:
 *
 * - Set the top value to SSM_TIMER32_TOP.
 * - Set the guard value to SSM_TIMER32_GUARD.
 * - Invoke the SSM_TIMER32_OVERFLOW_HANDLE() macro on counter overflow;
 *   this must be protected against interrupts.
 *
 * And only implement the ssm_timer32_* alternatives (where applicable).
 */

/** Initialize and start the timer device. */
extern int ssm_timer_start(void);

/** Set alarm for wake_time that triggers callback cb. Returns 0 on success. */
extern int ssm_timer_set_alarm(uint64_t wake_time, ssm_timer_callback_t cb,
                               void *user_data);
extern int ssm_timer32_set_alarm(uint32_t wake_time, ssm_timer32_callback_t cb,
                                 void *user_data);

/** Cancel previously set alarm. Returns 0 on success. */
extern int ssm_timer_cancel_alarm(void);

/** Read the current timer value. */
extern uint64_t ssm_timer_read(void);
extern uint32_t ssm_timer32_read(void);

#endif /* _SSM_PLATFORM_TIMER_H */
