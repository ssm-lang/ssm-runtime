#ifndef _BOARD_ZEPHYR_TIMER_H
#define _BOARD_ZEPHYR_TIMER_H

#include <drivers/counter.h>

/** @brief Board-specific Zephyr timer configuration.
 *
 *  @param ssm_timer_dev  timer device to initialize.
 *  @returns              0 on success, non-zero otherwise.
 */
int ssm_timer_board_start(const struct device *ssm_timer_dev);

#endif /* _BOARD_ZEPHYR_TIMER_H */
