#include <ssm.h>
#include <stdio.h>

void ssm_throw(int reason, const char *file, int line, const char *func) {
  printf("CUSTOM THROW\n");
}
