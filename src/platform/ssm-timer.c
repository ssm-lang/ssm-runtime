#ifndef PLATFORM_TIMER64
#include <platform/ssm-timer.h>
#include <platform/ssm-log.h>

SSM_LOG_NAME(ssm_timer64);

volatile uint32_t ssm_timer_hi;

static ssm_timer_callback_t timer_cb;

void ssm_timer32_callback(uint32_t time, void *user_data) {
  uint32_t hi0, lo, hi1;
  SSM_DEBUG_ASSERT(timer_cb, "ssm_timer32_callback: timer_cb not set\r\n");
  ssm_timer_read_to_from(&hi0, &lo, &hi1, time);
  timer_cb(ssm_timer_calc(hi0, lo, hi1), user_data);
}

int ssm_timer_set_alarm(uint64_t wake_time, ssm_timer_callback_t cb,
                        void *user_data) {
  if (wake_time - ssm_timer_read() > SSM_TIMER32_GUARD)
    return -EINVAL;

  // Overwriting the global is fine since we can only have one alarm at a time;
  // ssm_timer32_set_alarm should return an error if an alarm was already set.
  timer_cb = cb;

  return ssm_timer32_set_alarm(wake_time & SSM_TIMER32_TOP,
                               ssm_timer32_callback, user_data);
}

uint64_t ssm_timer_read(void) {
  uint32_t hi0, lo, hi1;
  ssm_timer_read_to(&hi0, &lo, &hi1);
  return ssm_timer_calc(hi0, lo, hi1);
}
#endif
