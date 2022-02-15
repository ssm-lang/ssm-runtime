#include "ssm-examples.h"
#include "ssm.h"
#include <stdio.h>

/* (print statements added in C implementation)

f x y z = x + y + z

main =
  let g = f 1
      h = g 2
      a = f 3 4 5
      b = g a 6
      c = h a
      d = h b

  a + b + c + d

should print:

  68
 */

typedef struct {
  ssm_act_t act;
  ssm_value_t x;
  ssm_value_t y;
  ssm_value_t z;
  ssm_value_t *__ret;
} act_f_t;

ssm_stepf_t step_f;

ssm_act_t *enter_f(ssm_act_t *parent, ssm_priority_t priority,
                   ssm_depth_t depth, ssm_value_t *argv, ssm_value_t *__ret) {
  act_f_t *cont =
      container_of(ssm_enter(sizeof(act_f_t), step_f, parent, priority, depth),
                   act_f_t, act);
  cont->x = argv[0];
  cont->y = argv[1];
  cont->z = argv[2];
  cont->__ret = __ret;
  return &cont->act;
}

void step_f(ssm_act_t *act) {
  act_f_t *cont = container_of(act, act_f_t, act);
  *cont->__ret = ssm_marshal(ssm_unmarshal(cont->x) + ssm_unmarshal(cont->y) +
                             ssm_unmarshal(cont->z));
  ssm_drop(cont->x);
  ssm_drop(cont->y);
  ssm_drop(cont->z);
  ssm_leave(&cont->act, sizeof(act_f_t));
}

typedef struct {
  ssm_act_t act;
  ssm_value_t f;
  ssm_value_t g;
  ssm_value_t h;
  ssm_value_t a2;
  ssm_value_t a1;
  ssm_value_t a;
  ssm_value_t b1;
  ssm_value_t b;
  ssm_value_t c;
  ssm_value_t d;
  ssm_value_t *__ret;
} act_main_t;

ssm_stepf_t step_main;

ssm_act_t *ssm_enter_main(struct ssm_act *parent, ssm_priority_t priority,
                          ssm_depth_t depth, ssm_value_t *argv,
                          ssm_value_t *ret) {
  act_main_t *cont = container_of(
      ssm_enter(sizeof(act_main_t), step_main, parent, priority, depth),
      act_main_t, act);
  cont->__ret = ret;

  return &cont->act;
}

void step_main(struct ssm_act *act) {
  act_main_t *cont = container_of(act, act_main_t, act);

  switch (act->pc) {
  case 0:;
    cont->f = ssm_new_closure(&enter_f, 3);

    ssm_closure_apply_auto(cont->f, ssm_marshal(1), act, act->priority,
                           act->depth, &cont->g);
    if (ssm_has_children(act)) {
      act->pc = 1;
      return;
    case 1:;
    }

    ssm_closure_apply_auto(cont->g, ssm_marshal(2), act, act->priority,
                           act->depth, &cont->h);
    if (ssm_has_children(act)) {
      act->pc = 2;
      return;
    case 2:;
    }

    ssm_closure_apply_auto(cont->f, ssm_marshal(3), act, act->priority,
                           act->depth, &cont->a2);
    if (ssm_has_children(act)) {
      act->pc = 3;
      return;
    case 3:;
    }
    ssm_closure_apply_auto(cont->a2, ssm_marshal(4), act, act->priority,
                           act->depth, &cont->a1);
    if (ssm_has_children(act)) {
      act->pc = 4;
      return;
    case 4:;
    }
    ssm_closure_apply_auto(cont->a1, ssm_marshal(5), act, act->priority,
                           act->depth, &cont->a);
    if (ssm_has_children(act)) {
      act->pc = 5;
      return;
    case 5:;
    }

    ssm_closure_apply_auto(cont->g, cont->a, act, act->priority, act->depth,
                           &cont->b1);
    if (ssm_has_children(act)) {
      act->pc = 6;
      return;
    case 6:;
    }

    ssm_closure_apply_auto(cont->b1, ssm_marshal(6), act, act->priority,
                           act->depth, &cont->b);
    if (ssm_has_children(act)) {
      act->pc = 7;
      return;
    case 7:;
    }

    ssm_closure_apply_auto(cont->h, cont->a, act, act->priority, act->depth,
                           &cont->c);
    if (ssm_has_children(act)) {
      act->pc = 8;
      return;
    case 8:;
    }

    ssm_closure_apply_auto(cont->h, cont->b, act, act->priority, act->depth,
                           &cont->d);
    if (ssm_has_children(act)) {
      act->pc = 9;
      return;
    case 9:;
    }

    *cont->__ret = ssm_marshal(ssm_unmarshal(cont->a) + ssm_unmarshal(cont->b) +
                               ssm_unmarshal(cont->c) + ssm_unmarshal(cont->d));
    ssm_drop(cont->d);
    ssm_drop(cont->c);
    ssm_drop(cont->b);
    ssm_drop(cont->b1);
    ssm_drop(cont->a);
    ssm_drop(cont->a1);
    ssm_drop(cont->a2);
    ssm_drop(cont->h);
    ssm_drop(cont->g);
    break;
  }
  ssm_leave(&cont->act, sizeof(*cont));
}

ssm_value_t ret = {.packed_val = 6969};
void ssm_program_init(void) {
  ssm_activate(ssm_enter_main(&ssm_top_parent, SSM_ROOT_PRIORITY,
                              SSM_ROOT_DEPTH, NULL, &ret));
}

void ssm_program_exit(void) {
  printf("Return value: %d\n", ssm_unmarshal(ret));
  ssm_drop(ret);
}
