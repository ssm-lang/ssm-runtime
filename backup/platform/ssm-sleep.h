#ifndef _PLATFORM_SSM_SLEEP_H
#define _PLATFORM_SSM_SLEEP_H

#include <platform-specific/ssm-sleep.h>

#ifndef SSM_MSLEEP
#error "SSM_MSLEEP not defined for platform"
#endif

#ifndef SSM_SLEEP
#define SSM_SLEEP(x) SSM_MSLEEP((x)*1000)
#endif

#endif /* ifndef _PLATFORM_SSM_SLEEP_H */
