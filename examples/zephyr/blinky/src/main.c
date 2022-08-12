#include <ssm-internal.h>
#include <kernel.h>
// #include <ssm-log.h>

// SSM_LOG_NAME(main);

/* #define SSM_TICK_STACKSIZE 4096 */
/* #define SSM_TICK_PRIORITY 7 */
/* K_THREAD_STACK_DEFINE(ssm_tick_thread_stack, SSM_TICK_STACKSIZE); */
/* struct k_thread ssm_tick_thread; */

/* #define SSM_LOG_STACKSIZE 256 */
/* #define SSM_LOG_PRIORITY 8 */
/* K_THREAD_STACK_DEFINE(ssm_log_thread_stack, SSM_LOG_STACKSIZE); */
/* struct k_thread ssm_log_thread; */

/* void ssm_log_body(void *p1, void *p2, void *p3) { */
/*    ssm_tick_log(); */
/* } */

/* void ssm_tick_body(void *p1, void* p2, void *p3) { */
/*   ssm_tick_loop(); */
/* } */

void main() {
  printk("starting\r\n");
  // LOG_INF("Sleeping for a second for you to start a terminal\r\n");
  k_sleep(K_SECONDS(1));
  printk("dummy main\r\n");

  ssm_platform_entry();

  /* LOG_INF("Starting...\r\n"); */

  /* k_thread_create(&ssm_log_thread, ssm_log_thread_stack, */
  /*                 K_THREAD_STACK_SIZEOF(ssm_log_thread_stack), */
  /*                 ssm_log_body, 0, 0, 0, SSM_LOG_PRIORITY, 0, */
  /*                 K_NO_WAIT); */

  /* k_thread_create(&ssm_tick_thread, ssm_tick_thread_stack, */
  /*                 K_THREAD_STACK_SIZEOF(ssm_tick_thread_stack), */
  /*                 ssm_tick_body, 0, 0, 0, SSM_TICK_PRIORITY, 0, */
  /*                 K_NO_WAIT); */
}
