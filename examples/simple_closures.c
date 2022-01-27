#include "ssm-examples.h"
#include <stdio.h>

/* (print statements added in C implementation)

main =
  let f x y = x + y
      g = f 1
  g 2 + g 3

should print:

  7
 */

typedef struct {
  ssm_act_t act;
  ssm_value_t x;
  ssm_value_t y;
  ssm_value_t *__ret;
} act_f_t;

ssm_stepf_t step_f;

ssm_act_t *enter_f(ssm_act_t *parent, ssm_priority_t priority,
                   ssm_depth_t depth, ssm_value_t *argv, ssm_value_t *__ret) {
  printf("entered f\n");
  act_f_t *cont = container_of(
      ssm_enter(sizeof(act_f_t), step_f, parent, priority, depth),
      act_f_t, act);
  cont->x = argv[0];
  cont->y = argv[1];
  cont->__ret = __ret;
  return &cont->act;
}

void step_f(ssm_act_t *act) {
  act_f_t *cont = container_of(act, act_f_t, act);

  printf("step f\n");
  *cont->__ret = ssm_new_time(ssm_time_read(cont->x) + ssm_time_read(cont->y));
  ssm_drop(cont->x);
  ssm_drop(cont->y);
  ssm_leave(&cont->act, sizeof(act_f_t));
}

typedef struct {
  ssm_act_t act;
  ssm_value_t f_closure;
  ssm_value_t g_closure;
  ssm_value_t t1;
  ssm_value_t t2;
  ssm_value_t t3;
  ssm_value_t __tmp0;
  ssm_value_t __tmp1;
} act_main_t;

ssm_stepf_t step_main;

ssm_act_t *ssm_enter_main(struct ssm_act *parent, ssm_priority_t priority,
                          ssm_depth_t depth) {
  act_main_t *cont = container_of(
      ssm_enter(sizeof(act_main_t), step_main, parent, priority, depth),
      act_main_t, act);

  return &cont->act;
}

void step_main(struct ssm_act *act) {
  act_main_t *cont = container_of(act, act_main_t, act);

  switch (act->pc) {
  case 0:;
    cont->f_closure = ssm_new_closure(&enter_f, 2);

    cont->t1 = ssm_new_time(1);
    cont->g_closure = cont->f_closure;
    // ssm_dup(cont->f_closure);
    ssm_closure_clone(cont->g_closure);
    // ssm_drop(cont->f_closure);

    ssm_closure_apply(cont->g_closure, cont->t1);
    ssm_drop(cont->t1);

    cont->t2 = ssm_new_time(2);
    ssm_closure_arg(cont->g_closure, ssm_closure_arg_count(cont->g_closure)) = cont->t2;
    printf("entering f\n");
    ssm_closure_activate(cont->g_closure, act, act->priority, act->depth, &cont->__tmp0);
    ssm_drop(cont->t2);
    act->pc = 1;
    printf("yielding to f\n");
    return;
  case 1:;
    cont->t3 = ssm_new_time(3);
    ssm_closure_arg(cont->g_closure, ssm_closure_arg_count(cont->g_closure)) = cont->t3;
    ssm_closure_activate(cont->g_closure, act, act->priority, act->depth, &cont->__tmp1);
    ssm_drop(cont->t3);
    ssm_drop(cont->g_closure);
    act->pc = 2;
    return;
  case 2:;
    printf("%ld\n", ssm_time_read(cont->__tmp0) + ssm_time_read(cont->__tmp1));
    ssm_drop(cont->__tmp0);
    ssm_drop(cont->__tmp1);
    break;
  }
  ssm_leave(&cont->act, sizeof(*cont));
}

void ssm_program_init(void) {
  ssm_activate(
      ssm_enter_main(&ssm_top_parent, SSM_ROOT_PRIORITY, SSM_ROOT_DEPTH));
}

void ssm_program_exit(void) { }
