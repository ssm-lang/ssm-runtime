/**
 * Mini SSM logging library.
 *
 * TODO: refactor to use "log" rather than "debug"
 */
#ifndef _PLATFORM_SSM_LOG_H
#define _PLATFORM_SSM_LOG_H

#include <platform-specific/ssm-log.h>

#include <ssm.h>

#ifndef SSM_LOG_NAME
#define SSM_LOG_NAME(name)
#endif

#ifndef SSM_DEBUG_PRINT
#define SSM_DEBUG_PRINT(...)                                                   \
  do                                                                           \
    ;                                                                          \
  while (0)
#endif

#ifndef SSM_DEBUG_ASSERT
#define SSM_DEBUG_ASSERT(cond, ...)                                            \
  do {                                                                         \
    if (!(cond)) {                                                             \
      SSM_DEBUG_PRINT(__VA_ARGS__);                                            \
      SSM_THROW(SSM_INTERNAL_ERROR);                                           \
    }                                                                          \
  } while (0)
#endif

/* "Disable" trace-related stuff. */
#ifndef SSM_DEBUG_TRACE
#define SSM_DEBUG_TRACE(...)                                                   \
  do                                                                           \
    ;                                                                          \
  while (0)
#endif

#ifndef SSM_DEBUG_MICROTICK
#define SSM_DEBUG_MICROTICK()                                                  \
  do                                                                           \
    ;                                                                          \
  while (0)
#endif

#endif /* _PLATFORM_SSM_LOG_H */
