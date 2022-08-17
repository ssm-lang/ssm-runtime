#include <ssm-internal.h>
#include <kernel.h>

void main(void) {
  printk("SSM Zephyr entry point\n");
  ssm_platform_entry();
}
