#include <platform/ssm-timer.h>
#include <platform/ssm-log.h>

#include <drivers/counter.h>
#include <drivers/clock_control.h>
#ifdef PLATFORM_BOARD_nrf52840_dk
#include <drivers/clock_control/nrf_clock_control.h>
#endif

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

#ifdef PLATFORM_BOARD_nrf52840_dk
static void show_clocks(void) {
  static const char *const lfsrc_s[] = {
#if defined(CLOCK_LFCLKSRC_SRC_LFULP)
    [NRF_CLOCK_LFCLK_LFULP] = "LFULP",
#endif
    [NRF_CLOCK_LFCLK_RC] = "LFRC",
    [NRF_CLOCK_LFCLK_Xtal] = "LFXO",
    [NRF_CLOCK_LFCLK_Synth] = "LFSYNT",
  };
  static const char *const hfsrc_s[] = {
      [NRF_CLOCK_HFCLK_LOW_ACCURACY] = "HFINT",
      [NRF_CLOCK_HFCLK_HIGH_ACCURACY] = "HFXO",
  };
  static const char *const clkstat_s[] = {
      [CLOCK_CONTROL_STATUS_STARTING] = "STARTING",
      [CLOCK_CONTROL_STATUS_OFF] = "OFF",
      [CLOCK_CONTROL_STATUS_ON] = "ON",
      [CLOCK_CONTROL_STATUS_UNAVAILABLE] = "UNAVAILABLE",
      [CLOCK_CONTROL_STATUS_UNKNOWN] = "UNKNOWN",
  };
  union {
    unsigned int raw;
    nrf_clock_lfclk_t lf;
    nrf_clock_hfclk_t hf;
  } src;
  enum clock_control_status clkstat;
  bool running;

  clkstat =
      clock_control_get_status(ssm_timer_dev, CLOCK_CONTROL_NRF_SUBSYS_LF);
  running = nrf_clock_is_running(NRF_CLOCK, NRF_CLOCK_DOMAIN_LFCLK, &src.lf);
  LOG_INF("LFCLK[%s]: %s %s ;", clkstat_s[clkstat], running ? "Running" : "Off",
          lfsrc_s[src.lf]);
  clkstat =
      clock_control_get_status(ssm_timer_dev, CLOCK_CONTROL_NRF_SUBSYS_HF);
  running = nrf_clock_is_running(NRF_CLOCK, NRF_CLOCK_DOMAIN_HFCLK, &src.hf);
  LOG_INF("HFCLK[%s]: %s %s\n", clkstat_s[clkstat], running ? "Running" : "Off",
          hfsrc_s[src.hf]);
}
#endif

int ssm_timer_start(void) {
  int err;
  struct counter_top_cfg top_cfg = {
      .callback = overflow_handler,
      .ticks = SSM_TIMER32_TOP,
      .user_data = NULL,
      .flags = COUNTER_TOP_CFG_DONT_RESET,
  };
#ifdef PLATFORM_BOARD_nrf52840_dk
  const struct device *clock;

  /* Configure nrf board to use external oscillator for timer */
  if (!(clock = device_get_binding(DT_LABEL(DT_INST(0, nordic_nrf_clock)))))
    return -ENODEV;

  if ((err = clock_control_on(clock, CLOCK_CONTROL_NRF_SUBSYS_HF)))
    return err;

  show_clocks();
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
  uint32_t hi0, lo, hi1;
  ssm_timer_read_to(&hi0, &lo, &hi1);
  return ssm_timer_calc(hi0, lo, hi1);
}
