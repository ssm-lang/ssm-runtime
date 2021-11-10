#ifndef _PLATFORM_SSM_SEM_H
#define _PLATFORM_SSM_SEM_H

#include <platform-specific/internal/ssm-sem.h>

#ifndef SSM_SEM_DEFINE
#error "SSM_SEM_DEFINE() not defined for platform"
#endif
#ifndef SSM_SEM_DECLARE
#error "SSM_SEM_DECLARE() not defined for platform"
#endif

#ifndef ssm_sem_post
#error "ssm_sem_post() not defined for platform"
#endif
#ifndef ssm_sem_wait
#error "ssm_sem_wait() not defined for platform"
#endif
#ifndef ssm_sem_reset
#error "ssm_sem_reset() not defined for platform"
#endif

#endif /* ifndef _PLATFORM_SSM_SEM_H */
