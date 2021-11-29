#include <ssm-types.h>
#include <ssm.h>
#include <stdio.h>
#include <stdlib.h>

/*
mywait (r: &a) =
  wait r

sum (r1: &Int) (r2: &Int) (r: &Int) =
  par mywait r1
      mywait r2
  after 1s, r <- deref r1 + deref r2

fib (n: Int) (&r: &Int) =
  let r1 = ref 0
  let r2 = ref 0
  if n < 2
    after 1s, r <- 1
  else
    par sum r1 r2 r
        fib (n - 1) r1
        fib (n - 2) r2

0 1 2 3 4 5  6  7  8  9 10  11  12  13
1 1 2 3 5 8 13 21 34 55 89 144 233 377
 */

typedef struct {
  ssm_act_t act;
  ssm_u32_t *r;
  struct ssm_trigger trigger1;
} act_mywait_t;

typedef struct {
  ssm_act_t act;
  ssm_u32_t *r1, *r2, *r;
} act_sum_t;

typedef struct {
  ssm_act_t act;
  u32 n;
  ssm_u32_t *r;
  ssm_u32_t r1, r2;
} act_fib_t;

ssm_stepf_t step_mywait;

ssm_act_t *ssm_enter_mywait(struct ssm_act *parent, ssm_priority_t priority,
                            ssm_depth_t depth, ssm_u32_t *r) {
  ssm_act_t *act =
      ssm_enter(sizeof(act_mywait_t), step_mywait, parent, priority, depth);
  act_mywait_t *cont = container_of(act, act_mywait_t, act);

  cont->trigger1.act = act;
  cont->r = r;
  return act;
}

void step_mywait(struct ssm_act *act) {
  act_mywait_t *cont = container_of(act, act_mywait_t, act);
  switch (act->pc) {
  case 0:
    ssm_sensitize(cont->r, &cont->trigger1);
    act->pc = 1;
    return;
  case 1:
    ssm_desensitize(&cont->trigger1);
    ssm_leave(act, sizeof(act_mywait_t));
    return;
  }
}

ssm_stepf_t step_sum;

ssm_act_t *ssm_enter_sum(struct ssm_act *parent, ssm_priority_t priority,
                         ssm_depth_t depth, ssm_u32_t *r1, ssm_u32_t *r2,
                         ssm_u32_t *r) {
  ssm_act_t *act =
      ssm_enter(sizeof(act_sum_t), step_sum, parent, priority, depth);
  act_sum_t *cont = container_of(act, act_sum_t, act);
  cont->r1 = r1;
  cont->r2 = r2;
  cont->r = r;
  return act;
}

void step_sum(struct ssm_act *act) {
  act_sum_t *cont = container_of(act, act_sum_t, act);
  ;
  switch (act->pc) {
  case 0: {
    ssm_depth_t new_depth = act->depth - 1; // 2 children
    ssm_priority_t new_priority = act->priority;
    ssm_priority_t pinc = 1 << new_depth;
    ssm_activate(ssm_enter_mywait(act, new_priority, new_depth, cont->r1));
    new_priority += pinc;
    ssm_activate(ssm_enter_mywait(act, new_priority, new_depth, cont->r2));
  }
    act->pc = 1;
    return;
  case 1:
    ssm_later(cont->r, ssm_now() + SSM_SECOND,
              ssm_marshal(ssm_unmarshal(cont->r1->value) +
                          ssm_unmarshal(cont->r2->value)));
    ssm_leave(act, sizeof(act_sum_t));
    return;
  }
}

ssm_stepf_t step_fib;

ssm_act_t *ssm_enter_fib(struct ssm_act *parent, ssm_priority_t priority,
                         ssm_depth_t depth, u32 n, ssm_u32_t *r) {
  ssm_act_t *act =
      ssm_enter(sizeof(act_fib_t), step_fib, parent, priority, depth);
  act_fib_t *cont = container_of(act, act_fib_t, act);

  cont->n = n;
  cont->r = r;
  ssm_initialize(&cont->r1);
  ssm_initialize(&cont->r2);
  return act;
}

void step_fib(struct ssm_act *act) {
  act_fib_t *cont = container_of(act, act_fib_t, act);
  switch (act->pc) {
  case 0:
    cont->r1.value = ssm_marshal(0);
    cont->r2.value = ssm_marshal(0);

    if (ssm_unmarshal(cont->n) < 2) {
      ssm_later(cont->r, ssm_now() + SSM_SECOND, ssm_marshal(1));
      ssm_leave(act, sizeof(act_fib_t));
      return;
    }
    {
      ssm_depth_t new_depth = act->depth - 2; // 4 children
      ssm_priority_t new_priority = act->priority;
      ssm_priority_t pinc = 1 << new_depth;
      ssm_activate(ssm_enter_fib(act, new_priority, new_depth,
                                 ssm_marshal(ssm_unmarshal(cont->n) - 1),
                                 &cont->r1));
      new_priority += pinc;
      ssm_activate(ssm_enter_fib(act, new_priority, new_depth,
                                 ssm_marshal(ssm_unmarshal(cont->n) - 2),
                                 &cont->r2));
      new_priority += pinc;
      ssm_activate(ssm_enter_sum(act, new_priority, new_depth, &cont->r1,
                                 &cont->r2, cont->r));
    }
    act->pc = 1;
    return;
  case 1:
    ssm_leave(act, sizeof(act_fib_t));
    return;
  }
}

void ssm_throw(int reason, const char *file, int line, const char *func) {
  printf("SSM error at %s:%s:%d: reason: %d\n", file, func, line, reason);
  exit(1);
}

int main(int argc, char *argv[]) {
  ssm_u32_t result;
  ssm_initialize(&result);
  result.value = ssm_marshal(0);

  int n = argc > 1 ? atoi(argv[1]) : 3;

  ssm_activate(ssm_enter_fib(&ssm_top_parent, SSM_ROOT_PRIORITY, SSM_ROOT_DEPTH,
                             ssm_marshal(n), &result));

  ssm_tick();

  while (ssm_next_event_time() != SSM_NEVER)
    ssm_tick();

  printf("simulated %lu seconds\n", ssm_now() / SSM_SECOND);
  printf("%d\n", (int)ssm_unmarshal(result.value));

  return 0;
}
