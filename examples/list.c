#include <ssm-internal.h>
#include <ssm-typedefs.h>
#include <ssm.h>
#include <stdio.h>

/* (print statements added in C implementation)

type List = Cons Int List | Nil

list = [1, 2, 3]

print_list (l: List) =
  match l
    Cons i l' = {{ print $ show i ++ "::" }}
                print_list l'
    Nil = {{ print "[]\r\n" }}

map_inc (l: List) -> List =
  match l
    Nil = Nil
    Cons i l' = Cons (i + 1) (map_inc l')

main =
  print_list list
  let list' = map_inc list
  print_list list'

should print:

  1::2::3::[]
  2::3::4::[]
 */

enum { List_size = 2 };
typedef struct {
  struct ssm_mm mm;
  ssm_value_t payload[2];
} List;
enum List { Nil = 0, Cons };

List __0_list = {.mm = {.val_count = List_size, .tag = Nil, .ref_count = 1}};
List __1_list = {
    .mm = {.val_count = List_size, .tag = Cons, .ref_count = 1},
    .payload = {
        [0] = ssm_marshal_static(3), [1] = ssm_from_obj_static(&__0_list)}};
List __2_list = {
    .mm = {.val_count = List_size, .tag = Cons, .ref_count = 1},
    .payload = {
        [0] = ssm_marshal_static(2), [1] = ssm_from_obj_static(&__1_list)}};
List __3_list = {
    .mm = {.val_count = List_size, .tag = Cons, .ref_count = 1},
    .payload = {
        [0] = ssm_marshal_static(1), [1] = ssm_from_obj_static(&__2_list)}};
ssm_value_t list = ssm_from_obj_static(&__3_list);

typedef struct {
  ssm_act_t act;
  ssm_value_t l;
  ssm_value_t __tmp0;
  ssm_value_t __tmp1;
  ssm_value_t *__ret;
} act_map_inc_t;

ssm_stepf_t step_map_inc;

ssm_act_t *enter_map_inc(ssm_act_t *parent, ssm_priority_t priority,
                         ssm_depth_t depth, ssm_value_t l, ssm_value_t *__ret) {
  act_map_inc_t *cont = container_of(
      ssm_enter(sizeof(act_map_inc_t), step_map_inc, parent, priority, depth),
      act_map_inc_t, act);
  cont->l = l;
  cont->__ret = __ret;
  return &cont->act;
}

void step_map_inc(ssm_act_t *act) {
  act_map_inc_t *cont = container_of(act, act_map_inc_t, act);

  switch (act->pc) {
  case 0:
    switch (ssm_to_obj(cont->l)->mm.tag) {
    case Nil:
      goto match_Nil_0;
    case Cons:
      goto match_Cons_0;
    }
    SSM_ASSERT(0);
  match_Nil_0:
    ssm_drop(&ssm_to_obj(cont->l)->mm);
    *cont->__ret = ssm_from_obj(ssm_new(List_size, Nil));
    break;
  match_Cons_0:;
    ssm_value_t __tmp0 = ssm_to_obj(cont->l)->payload[0];
    ssm_value_t __tmp1 = ssm_to_obj(cont->l)->payload[1];
    ssm_dup(&ssm_to_obj(__tmp1)->mm);

    ssm_drop(&ssm_to_obj(cont->l)->mm);

    cont->__tmp0 = ssm_marshal(ssm_unmarshal(__tmp0) + 1);

    ssm_activate(enter_map_inc(act, act->priority, act->depth,
                               ssm_to_obj(cont->l)->payload[1], &cont->__tmp1));
    act->pc = 1;
    return;
  case 1:;
    struct ssm_object *__tmp3 = ssm_new(List_size, Cons);
    __tmp3->payload[0] = cont->__tmp0;
    __tmp3->payload[1] = cont->__tmp1;
    *cont->__ret = ssm_from_obj(__tmp3);
    break;
  }
  ssm_leave(&cont->act, sizeof(act_map_inc_t));
}

typedef struct {
  ssm_act_t act;
  ssm_value_t l;
  ssm_value_t __tmp0;
  ssm_value_t __tmp1;
} act_print_list_t;

ssm_stepf_t step_print_list;

ssm_act_t *enter_print_list(ssm_act_t *parent, ssm_priority_t priority,
                            ssm_depth_t depth, ssm_value_t l) {
  act_print_list_t *cont =
      container_of(ssm_enter(sizeof(act_print_list_t), step_print_list, parent,
                             priority, depth),
                   act_print_list_t, act);
  cont->l = l;
  return &cont->act;
}

void step_print_list(ssm_act_t *act) {
  act_print_list_t *cont = container_of(act, act_print_list_t, act);
  switch (act->pc) {
  case 0:
    switch (ssm_to_obj(cont->l)->mm.tag) {
    case Nil:
      goto match_Nil_0;
    case Cons:
      goto match_Cons_0;
    }
    SSM_ASSERT(0);
  match_Nil_0:
    printf("[]\r\n");
    ssm_drop(&ssm_to_obj(cont->l)->mm);
    break;
  match_Cons_0:;
    ssm_value_t __tmp0 = ssm_to_obj(cont->l)->payload[0];
    ssm_value_t __tmp1 = ssm_to_obj(cont->l)->payload[1];
    ssm_dup(&ssm_to_obj(__tmp1)->mm);

    ssm_drop(&ssm_to_obj(cont->l)->mm);

    printf("%ld::", ssm_unmarshal(__tmp0));

    ssm_activate(enter_print_list(act, act->priority, act->depth, __tmp1));
    act->pc = 1;
    return;
  case 1:
    break;
  }
  ssm_leave(&cont->act, sizeof(act_print_list_t));
}

typedef struct {
  ssm_act_t act;
  ssm_value_t list;
  ssm_value_t list_;
} act_main_t;

ssm_stepf_t step_main;

// Create a new activation record for main
ssm_act_t *ssm_enter_main(struct ssm_act *parent, ssm_priority_t priority,
                          ssm_depth_t depth) {
  act_main_t *cont = container_of(
      ssm_enter(sizeof(act_main_t), step_main, parent, priority, depth),
      act_main_t, act);

  ssm_dup(&ssm_to_obj(list)->mm); // capture global by closure
  cont->list = list;
  return &cont->act;
}

void step_main(struct ssm_act *act) {
  act_main_t *cont = container_of(act, act_main_t, act);

  switch (act->pc) {
  case 0:
    ssm_dup(&ssm_to_obj(cont->list)->mm);
    ssm_activate(enter_print_list(act, act->priority, act->depth, cont->list));
    act->pc = 1;
    return;
  case 1:
    ssm_activate(enter_map_inc(act, act->priority, act->depth, cont->list,
                               &cont->list_));
    act->pc = 2;
    return;
  case 2:
    ssm_activate(enter_print_list(act, act->priority, act->depth, cont->list_));
    act->pc = 3;
    return;
  case 3:
    break;
  }
  ssm_leave(&cont->act, sizeof(*cont));
}

void ssm_throw(enum ssm_error reason, const char *file, int line,
               const char *func) {
  printf("SSM error at %s:%s:%d: reason: %d\n", file, func, line, reason);
  exit(1);
}

int main(int argc, char *argv[]) {
  ssm_time_t stop_at = (argc > 1 ? atoi(argv[1]) : 20) * SSM_SECOND;

  ssm_act_t *act =
      ssm_enter_main(&ssm_top_parent, SSM_ROOT_PRIORITY, SSM_ROOT_DEPTH);
  ssm_activate(act);

  ssm_tick();

  while (ssm_next_event_time() != SSM_NEVER && ssm_now() < stop_at)
    ssm_tick();

  printf("simulated %lu seconds\n", ssm_now() / SSM_SECOND);

  return 0;
}
