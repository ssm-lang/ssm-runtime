#include <ssm-types.h>
#include <ssm.h>
#include <stdio.h>

/* Simple time keeper

second_clock (second_event: &()) =
  let timer = ref ()
  loop
    second_event <- ()
    after 1s, timer <- ()
    wait timer

report_seconds (second_event: &()) =
  let seconds = ref 0
  loop
    wait second_event
    seconds <- deref seconds + 1
    print(deref seconds)

main =
  let second = ref ()
  par second_clock second
      report_seconds second
 */

typedef struct {
  ssm_act_t act;
  ssm_event_t *second_event;
  ssm_event_t timer;
  struct ssm_trigger trigger1;
} act_second_clock_t;

typedef struct {
  ssm_act_t act;
  ssm_event_t *second_event;
  ssm_i32_t seconds;
  struct ssm_trigger trigger1;
} act_report_seconds_t;

typedef struct {
  ssm_act_t act;
  ssm_event_t second;
} act_main_t;

ssm_stepf_t step_second_clock;

ssm_act_t *ssm_enter_second_clock(struct ssm_act *parent,
                                  ssm_priority_t priority, ssm_depth_t depth,
                                  ssm_event_t *second_event) {
  ssm_act_t *cont = ssm_enter(sizeof(act_second_clock_t), step_second_clock,
                              parent, priority, depth);
  act_second_clock_t *act = container_of(cont, act_second_clock_t, act);
  act->second_event = second_event;
  ssm_initialize(&act->timer);
  return cont;
}

void step_second_clock(struct ssm_act *cont) {
  act_second_clock_t *act = container_of(cont, act_second_clock_t, act);
  switch (cont->pc) {
  case 0:
    for (;;) {
      ssm_assign(act->second_event, cont->priority, EVENT_VALUE);
      ssm_later(&act->timer, ssm_now() + SSM_SECOND, EVENT_VALUE);
      act->trigger1.act = cont;
      ssm_sensitize(&act->timer, &act->trigger1);
      cont->pc = 1;
      return;
    case 1:
      ssm_desensitize(&act->trigger1);
    }
  }
  ssm_leave(cont, sizeof(act_second_clock_t));
}

ssm_stepf_t step_report_seconds;

ssm_act_t *ssm_enter_report_seconds(struct ssm_act *parent,
                                    ssm_priority_t priority, ssm_depth_t depth,
                                    ssm_event_t *second_event) {
  ssm_act_t *cont = ssm_enter(sizeof(act_report_seconds_t), step_report_seconds,
                              parent, priority, depth);
  act_report_seconds_t *act = container_of(cont, act_report_seconds_t, act);
  act->second_event = second_event;
  ssm_initialize(&act->seconds);
  act->seconds.value = ssm_marshal(0);
  return cont;
}

void step_report_seconds(struct ssm_act *cont) {
  act_report_seconds_t *act = container_of(cont, act_report_seconds_t, act);

  switch (cont->pc) {
  case 0:
    for (;;) {
      act->trigger1.act = cont;
      ssm_sensitize(act->second_event, &act->trigger1);
      cont->pc = 1;
      return;
    case 1:
      ssm_desensitize(&act->trigger1);
      ssm_assign(&act->seconds, cont->priority,
                 ssm_marshal(ssm_unmarshal(act->seconds.value) + 1));

      printf("%d\n", (int)ssm_unmarshal(act->seconds.value));
    }
  }
  ssm_leave(cont, sizeof(act_report_seconds_t));
}

ssm_stepf_t step_main;

// Create a new activation record for main
ssm_act_t *ssm_enter_main(struct ssm_act *parent, ssm_priority_t priority,
                          ssm_depth_t depth) {
  ssm_act_t *cont =
      ssm_enter(sizeof(act_main_t), step_main, parent, priority, depth);
  act_main_t *act = container_of(cont, act_main_t, act);

  ssm_initialize(&act->second);
  return cont;
}

void step_main(struct ssm_act *cont) {
  act_main_t *act = container_of(cont, act_main_t, act);

  switch (cont->pc) {
  case 0: {
    ssm_depth_t new_depth = cont->depth - 1;
    ssm_priority_t new_priority = cont->priority;
    ssm_priority_t pinc = 1 << new_depth;
    ssm_activate(
        ssm_enter_second_clock(cont, new_priority, new_depth, &act->second));
    new_priority += pinc;
    ssm_activate(
        ssm_enter_report_seconds(cont, new_priority, new_depth, &act->second));
  }
    cont->pc = 1;
    return;
  case 1:
    ssm_leave(cont, sizeof(act_main_t));
    return;
  }
}

void ssm_throw(int reason, const char *file, int line, const char *func) {
  printf("SSM error at %s:%s:%d: reason: %d\n", file, func, line, reason);
  exit(1);
}

int main(int argc, char *argv[]) {
  ssm_time_t stop_at = (argc > 1 ? atoi(argv[1]) : 20) * SSM_SECOND;

  ssm_act_t *cont =
      ssm_enter_main(&ssm_top_parent, SSM_ROOT_PRIORITY, SSM_ROOT_DEPTH);
  ssm_activate(cont);

  ssm_tick();

  while (ssm_next_event_time() != SSM_NEVER && ssm_now() < stop_at)
    ssm_tick();

  printf("simulated %lu seconds\n", ssm_now() / SSM_SECOND);

  return 0;
}
