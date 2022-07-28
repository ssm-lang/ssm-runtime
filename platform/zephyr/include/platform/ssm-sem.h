/**
 * Wrapper around Zephyr's semaphore API
 */
#ifndef _PLATFORM_SSM_SEM_H
#define _PLATFORM_SSM_SEM_H

#include <kernel.h>

// Any way to declare this as static? */
#define SSM_SEM_DEFINE(name) K_SEM_DEFINE(name, 0, 1)
#define SSM_SEM_DECLARE(name) extern struct k_sem name

#define ssm_sem_post(sem) k_sem_give(sem)
#define ssm_sem_wait(sem) k_sem_take(sem, K_FOREVER)
#define ssm_sem_reset(sem) k_sem_reset(sem)

#endif /* ifndef _PLATFORM_SSM_SEM_H */
