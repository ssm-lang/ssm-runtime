/** @file ssm-mem.c
 *  @brief SSM runtime memory management.
 *
 *
 *  @author John Hui (j-hui)
 */
#include <ssm-internal.h>

struct ssm_mm_header *ssm_alloc(size_t size) {
  // TODO: implement this.
}

void ssm_free(struct ssm_mm_header *mm) {
  // TODO: implement this.
}

ssm_value_t ssm_new(size_t size) {
  // TODO: implement this.
}

void ssm_dup(ssm_value_t v) {
  // TODO: implement this.
}

void ssm_drop(ssm_value_t v) {
  // TODO: implement this.
}

ssm_value_t ssm_reuse(ssm_value_t v) {
  // TODO: implement this.
}
