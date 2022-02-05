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
  struct ssm_closure1 *closure = container_of(
      ssm_mem_alloc(ssm_closure_size(arg_cap)), struct ssm_closure1, mm);
  closure->mm.ref_count = 1;
  closure->mm.kind = SSM_CLOSURE_K;
  closure->mm.val_count = 0;
  closure->mm.tag = arg_cap;
  closure->f = f;
  return (ssm_value_t){.heap_ptr = &closure->mm};
}

// These are functions rather than macros to limit code size.
// I anticipate few valuable opportunities for the optimizer to inline
// code, since it's quite difficult to know ahead of time how many
// arguments are already contained in a closure.
void ssm_closure_dup_args(ssm_value_t closure) {
  for (size_t i = 0; i < ssm_closure_arg_count(closure); i++)
    ssm_dup(ssm_closure_arg(closure, i));
}

void ssm_closure_dup_argv(ssm_value_t closure) {
  for (size_t i = 0; i < ssm_closure_arg_cap(closure); i++)
    ssm_dup(ssm_closure_arg(closure, i));
}

void ssm_closure_drop_args(ssm_value_t closure) {
  for (size_t i = 0; i < ssm_closure_arg_count(closure); i++)
    ssm_drop(ssm_closure_arg(closure, i));
}

ssm_value_t ssm_closure_apply_unsafe(ssm_value_t closure, ssm_value_t arg) {
  ssm_value_t new_closure =
      ssm_new_closure(ssm_closure_func(closure), ssm_closure_arg_cap(closure));

  for (size_t i = 0; i < ssm_closure_arg_count(closure); i++) {
    ssm_dup(ssm_closure_arg(closure, i));
    ssm_closure_arg(closure, ssm_closure_arg_count(closure)) = arg;
  }

  ssm_closure_arg(closure, ssm_closure_arg_count(closure)) = arg;

  return new_closure;
}
