#include <platform/zephyr-timer.h>
#include <logging/log.h>

LOG_MODULE_DECLARE(ssm_timer);

int ssm_timer_board_start(const struct device *ssm_timer_dev) {

  printk("Not yet implemented, using default clock (probably inaccurate)\n");
  // const struct device *clock;
  // int err;
  //
  // /* Configure nrf board to use external oscillator for timer */
  // if (!(clock = device_get_binding(DT_LABEL(DT_INST(0, nordic_nrf_clock)))))
  //   return -ENODEV;
  //
  // if ((err = clock_control_on(clock, CLOCK_CONTROL_NRF_SUBSYS_HF)))
  //   return err;
  //
  // show_clocks(ssm_timer_dev);

  return 0;
}
