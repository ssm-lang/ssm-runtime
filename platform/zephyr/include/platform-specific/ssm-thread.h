#ifndef _PLATFORM_SPECIFIC_SSM_THREAD_H
#define _PLATFORM_SPECIFIC_SSM_THREAD_H

#include <kernel.h>

// Not sure if these should be configurable?
#define SSM_TICK_STACKSIZE 4096
#define SSM_TICK_PRIORITY 7

#define ssm_thread_create(thread_body)                                           \
  do {                                                                           \
    static K_THREAD_STACK_DEFINE(ssm_tick_thread_stack, SSM_TICK_STACKSIZE);     \
    static struct k_thread ssm_tick_thread;                                      \
    k_thread_create(&ssm_tick_thread, ssm_tick_thread_stack,                     \
                    K_THREAD_STACK_SIZEOF(ssm_tick_thread_stack),                \
                    thread_body, 0, 0, 0, SSM_TICK_PRIORITY, 0,                  \
                    K_NO_WAIT);                                                  \
  } while (0)

#endif /* ifndef _PLATFORM_SPECIFIC_SSM_THREAD_H */
