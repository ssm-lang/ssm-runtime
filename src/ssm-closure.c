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

ssm_value_t ssm_new_closure(ssm_func_t f) {
  struct ssm_mm *mm = ssm_mem_alloc(ssm_closure_size(0));
  struct ssm_closure1 *closure = container_of(mm, struct ssm_closure1, mm);
  mm->ref_count = 1;
  mm->kind = SSM_CLOSURE_K;
  mm->val_count = 0;
  closure->f = f;
  return (ssm_value_t){.heap_ptr = mm};
}

ssm_value_t ssm_closure_apply(ssm_value_t closure, ssm_value_t arg) {
  uint8_t val_count = ssm_closure_val_count(closure);
  struct ssm_mm *mm = ssm_mem_alloc(ssm_closure_size(val_count + 1));
  struct ssm_closure1 *applied_closure = container_of(mm, struct ssm_closure1,
                                                      mm);
  mm->ref_count = 1;
  mm->kind = SSM_CLOSURE_K;
  mm->val_count = val_count + 1;
  applied_closure->f = ssm_closure_func(closure);
  for (size_t i = 0; i < val_count; i++) {
    ssm_value_t arg_i = ssm_closure_arg(closure, i);
    ssm_dup(arg_i);
    applied_closure->argv[i] = arg_i;
  }
  applied_closure->argv[val_count] = arg;
  ssm_dup(arg);
  return (ssm_value_t){.heap_ptr = mm};
}

ssm_act_t *ssm_closure_reduce(ssm_value_t closure, ssm_value_t arg,
                              ssm_act_t *parent, ssm_priority_t prio,
                              ssm_depth_t depth, ssm_value_t *ret) {
  ssm_value_t val = ssm_closure_apply(closure, arg);
  ssm_act_t *act = ssm_closure_func(val)(parent, prio, depth,
                                         ssm_closure_argv(val), ret);
  ssm_drop(val); // TODO(hans): Feels weird to allocate just to drop..
  return act;
}
