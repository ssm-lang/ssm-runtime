/** @file ssm-closure.c
 *  @brief SSM closure management and allocation.
 *
 *  @author Hans Montero (hmontero1205)
 */
#include <assert.h>
#include <ssm-internal.h>
#include <ssm.h>
#include <stdio.h>
#include <stdlib.h>

ssm_value_t ssm_new_closure(ssm_func_t f, uint8_t arg_cap) {
  struct ssm_mm *mm = ssm_mem_alloc(ssm_closure_size(arg_cap));
  struct ssm_closure1 *closure = container_of(mm, struct ssm_closure1, mm);
  mm->ref_count = 1;
  mm->kind = SSM_CLOSURE_K;
  mm->val_count = 0;
  mm->tag = arg_cap;
  closure->f = f;
  return (ssm_value_t){.heap_ptr = mm};
}

// TODO: figure out if the overhead of this function call is worth it
void ssm_closure_dup_args(ssm_value_t closure) {
  for (size_t i = 0; i < ssm_closure_arg_count(closure); i++)
    ssm_dup(ssm_closure_arg(closure, i));
}

// TODO: figure out if the overhead of this function call is worth it
void ssm_closure_drop_args(ssm_value_t closure) {
  for (size_t i = 0; i < ssm_closure_arg_count(closure); i++)
    ssm_drop(ssm_closure_arg(closure, i));
}

ssm_value_t ssm_closure_clone_unsafe(ssm_value_t old_closure) {
  ssm_value_t new_closure = ssm_new_closure(ssm_closure_func(old_closure),
                                            ssm_closure_arg_cap(old_closure));
  for (size_t i = 0; i < ssm_closure_arg_cap(old_closure); i++)
    ssm_closure_apply_unsafe(new_closure, ssm_closure_arg(old_closure, i));
  return new_closure;
}
