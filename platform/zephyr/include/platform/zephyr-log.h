#ifndef _PLATFORM_ZEPHYR_LOG_H
#define _PLATFORM_ZEPHYR_LOG_H

#include <logging/log.h>
#include <logging/log_ctrl.h>

#define SSM_LOG_NAME(name) LOG_MODULE_REGISTER(name)
#define SSM_DEBUG_PRINT(...) LOG_DBG(__VA_ARGS__)

#endif /* ifndef _PLATFORM_ZEPHYR_LOG_H */
