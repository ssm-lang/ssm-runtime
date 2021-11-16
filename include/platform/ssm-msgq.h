#ifndef _PLATFORM_SSM_MSGQ_H
#define _PLATFORM_SSM_MSGQ_H

#include <platform-specific/ssm-msgq.h>

#ifndef SSM_MSGQ_DEFINE
#error "SSM_MSGQ_DEFINE() not defined for platform"
#endif

#ifndef ssm_msgq_put
#error "ssm_msgq_put() not defined for platform"
#endif

#ifndef ssm_msgq_get
#error "ssm_msgq_get() not defined for platform"
#endif

#endif /* ifndef _PLATFORM_SSM_MSGQ_H */
