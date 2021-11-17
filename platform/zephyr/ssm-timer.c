#include <platform/ssm-timer.h>
#include <platform/ssm-log.h>
#include <board-specific/ssm-timer.h>

#include <drivers/counter.h>
#include <drivers/clock_control.h>

#include <logging/log.h>

LOG_MODULE_REGISTER(ssm_timer);

#if !DT_NODE_HAS_STATUS(DT_ALIAS(ssm_timer), okay)
#error "ssm-timer device is not supported on this board"
#endif

#define SSM_TIMER_ALARM_CHANNEL 0

const struct device *ssm_timer_dev;

static void overflow_handler(const struct device *dev, void *user_data) {
  uint32_t key = irq_lock();
  SSM_TIMER32_OVERFLOW_HANDLE();
  irq_unlock(key);
}

int ssm_timer_start(void) {
  int err;
  struct counter_top_cfg top_cfg = {
      .callback = overflow_handler,
      .ticks = SSM_TIMER32_TOP,
      .user_data = NULL,
      .flags = COUNTER_TOP_CFG_DONT_RESET,
  };
#ifdef BOARD_TIMER_INIT
  // This macro may return from within
  BOARD_TIMER_INIT(ssm_timer_dev);
#endif

  if (!(ssm_timer_dev = device_get_binding(DT_LABEL(DT_ALIAS(ssm_timer)))))
    return -ENODEV;

  if (!(SSM_TIMER32_TOP <= counter_get_max_top_value(ssm_timer_dev)))
    return -ENOTSUP;

  LOG_INF("timer will run at %d Hz\r\n", counter_get_frequency(ssm_timer_dev));
  LOG_INF("timer will wraparound at %08x ticks\r\n", SSM_TIMER32_TOP);

  ssm_timer_hi = 0;

  if ((err = counter_set_top_value(ssm_timer_dev, &top_cfg)))
    return err;

  if ((err = counter_set_guard_period(ssm_timer_dev, SSM_TIMER32_GUARD,
                                      COUNTER_GUARD_PERIOD_LATE_TO_SET)))
    return err;

  return counter_start(ssm_timer_dev);
}

static ssm_timer32_callback_t timer_cb;

void counter_alarm_callback(const struct device *dev,
	                    uint8_t chan_id, uint32_t ticks, void *user_data) {
  SSM_DEBUG_ASSERT(chan_id == SSM_TIMER_ALARM_CHANNEL,
                   "counter_alarm_callback: unexpected chan_id: %u\r\n", chan_id);
  SSM_DEBUG_ASSERT(timer_cb, "counter_alarm_callback: timer_cb not set\r\n");
  timer_cb(ticks, user_data);
}

int ssm_timer32_set_alarm(uint32_t wake_time, ssm_timer32_callback_t cb,
                          void *user_data) {
  struct counter_alarm_cfg cfg;

  // Overwriting the global is fine since we can only have one alarm at a time;
  // counter_set_channel_alarm should return an error if an alarm was already set.
  timer_cb = cb;

  cfg.flags = COUNTER_ALARM_CFG_ABSOLUTE | COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE;
  cfg.ticks = wake_time;
  cfg.callback = counter_alarm_callback;
  cfg.user_data = user_data;
  return counter_set_channel_alarm(ssm_timer_dev, SSM_TIMER_ALARM_CHANNEL,
                                   &cfg);
}

int ssm_timer_cancel_alarm(void) {
  return counter_cancel_channel_alarm(ssm_timer_dev, SSM_TIMER_ALARM_CHANNEL);
}

uint32_t ssm_timer32_read(void) {
  uint32_t ticks;
  counter_get_value(ssm_timer_dev, &ticks);
  return ticks;
}
