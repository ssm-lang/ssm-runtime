/** @file ssm-mem.c
 *  @brief SSM runtime memory management.
 *
 *  @author John Hui (j-hui)
 */
#include <ssm-internal.h>

struct ssm_mm *ssm_mem_alloc(size_t size) {
  return malloc(size); // TODO: (dan) use our own allocator
}

void ssm_mem_free(struct ssm_mm *mm, size_t size) {
  free(mm); // TODO: (dan) use our own allocator
}

struct ssm_mm *ssm_new_builtin(enum ssm_builtin builtin) {
  struct ssm_mm *mm = ssm_mem_alloc(SSM_BUILTIN_SIZE(builtin));
  mm->val_count = SSM_BUILTIN;
  mm->tag = builtin;
  mm->ref_count = 0;
  return mm;
}

struct ssm_object *ssm_new(uint8_t val_count, uint8_t tag) {
  SSM_ASSERT(val_count > 0);
  struct ssm_mm *mm = ssm_mem_alloc(SSM_OBJ_SIZE(val_count));
  mm->val_count = val_count;
  mm->tag = tag;
  mm->ref_count = 1;
  return container_of(mm, struct ssm_object, mm);
}

void ssm_dup(struct ssm_mm *mm) { ++mm->ref_count; }

void ssm_drop(struct ssm_mm *mm) {
  // TODO: implement this.
}

struct ssm_mm *ssm_reuse(struct ssm_mm *mm) {
  return NULL; // TODO: implement this.
}

void ssm_free(struct ssm_mm *mm) {
  ssm_mem_free(mm, ssm_mm_is_builtin(mm) ? SSM_BUILTIN_SIZE(mm->tag)
                                         : SSM_OBJ_SIZE(mm->val_count));
}
