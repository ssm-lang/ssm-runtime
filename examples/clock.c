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
  act_second_clock_t *cont = malloc(sizeof(*cont));
  ssm_enter(&cont->act, step_second_clock, parent, priority, depth);
  cont->second_event = second_event;
  ssm_initialize(&cont->timer);
  return &cont->act;
}

void step_second_clock(struct ssm_act *act) {
  act_second_clock_t *cont = container_of(act, act_second_clock_t, act);
  switch (act->pc) {
  case 0:
    for (;;) {
      ssm_assign(cont->second_event, act->priority, EVENT_VALUE);
      ssm_later(&cont->timer, ssm_now() + SSM_SECOND, EVENT_VALUE);
      cont->trigger1.act = act;
      ssm_sensitize(&cont->timer, &cont->trigger1);
      act->pc = 1;
      return;
    case 1:
      ssm_desensitize(&cont->trigger1);
    }
  }
  act = ssm_leave(&cont->act);
  free(cont);
  if (act)
    ssm_call(act);
}

ssm_stepf_t step_report_seconds;

ssm_act_t *ssm_enter_report_seconds(struct ssm_act *parent,
                                    ssm_priority_t priority, ssm_depth_t depth,
                                    ssm_event_t *second_event) {
  act_report_seconds_t *cont = malloc(sizeof(*cont));
  ssm_enter(&cont->act, step_report_seconds, parent, priority, depth);
  cont->second_event = second_event;
  ssm_initialize(&cont->seconds);
  cont->seconds.value = ssm_marshal(0);
  return &cont->act;
}

void step_report_seconds(struct ssm_act *act) {
  act_report_seconds_t *cont = container_of(act, act_report_seconds_t, act);

  switch (act->pc) {
  case 0:
    for (;;) {
      cont->trigger1.act = act;
      ssm_sensitize(cont->second_event, &cont->trigger1);
      act->pc = 1;
      return;
    case 1:
      ssm_desensitize(&cont->trigger1);
      ssm_assign(&cont->seconds, act->priority,
                 ssm_marshal(ssm_unmarshal(cont->seconds.value) + 1));

      printf("%d\n", (int)ssm_unmarshal(cont->seconds.value));
    }
  }
  act = ssm_leave(&cont->act);
  free(cont);
  if (act)
    ssm_call(act);
}

ssm_stepf_t step_main;

// Create a new activation record for main
ssm_act_t *ssm_enter_main(struct ssm_act *parent, ssm_priority_t priority,
                          ssm_depth_t depth) {
  act_main_t *cont = malloc(sizeof(*cont));
  ssm_enter(&cont->act, step_main, parent, priority, depth);

  ssm_initialize(&cont->second);
  return &cont->act;
}

void step_main(struct ssm_act *act) {
  act_main_t *cont = container_of(act, act_main_t, act);

  switch (act->pc) {
  case 0: {
    ssm_depth_t new_depth = act->depth - 1;
    ssm_priority_t new_priority = act->priority;
    ssm_priority_t pinc = 1 << new_depth;
    ssm_activate(
        ssm_enter_second_clock(act, new_priority, new_depth, &cont->second));
    new_priority += pinc;
    ssm_activate(
        ssm_enter_report_seconds(act, new_priority, new_depth, &cont->second));
  }
    act->pc = 1;
    return;
  case 1:
    break;
  }
  act = ssm_leave(&cont->act);
  free(cont);
  if (act)
    ssm_call(act);
}

void ssm_throw(int reason, const char *file, int line, const char *func) {
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
