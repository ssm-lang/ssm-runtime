/**
 * Wrapper around Zephyrs sleep
 */
#ifndef _PLATFORM_SSM_SLEEP_H
#define _PLATFORM_SSM_SLEEP_H

#include <kernel.h>

#define SSM_MSLEEP(ms) k_sleep(K_MSEC(ms))
#define SSM_SLEEP(ms) k_sleep(K_SECONDS(ms))

#endif /* ifndef _PLATFORM_SSM_SLEEP_H */
