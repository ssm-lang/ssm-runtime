#include "ssm.h"
#include <ctype.h>
#include <execinfo.h>
#include <pthread.h>
#include <ssm-internal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <sys/eventfd.h>
#include <sys/select.h>
#include <time.h>
#include <unistd.h>

#ifndef SSM_TIMER64_PRESENT
#error This example needs to be compiled with SSM_TIMER64_PRESENT
#endif

#define NANOS 1000000000L

#define timespec_diff(a, b)                                                    \
  (a).tv_nsec - (b).tv_nsec < 0                                                \
      ? (struct timespec){.tv_sec = (a).tv_sec - (b).tv_sec - 1,               \
                          .tv_nsec = (a).tv_nsec - (b).tv_nsec + NANOS}        \
      : (struct timespec) {                                                    \
    .tv_sec = (a).tv_sec - (b).tv_sec, .tv_nsec = (a).tv_nsec - (b).tv_nsec    \
  }

#define timespec_add(a, b)                                                     \
  (a).tv_nsec + (b).tv_nsec > NANOS                                            \
      ? (struct timespec){.tv_sec = (a).tv_sec + (b).tv_sec + 1,               \
                          .tv_nsec = (a).tv_nsec + (b).tv_nsec - NANOS}        \
      : (struct timespec) {                                                    \
    .tv_sec = (a).tv_sec + (b).tv_sec, .tv_nsec = (a).tv_nsec + (b).tv_nsec    \
  }

#define timespec_lt(a, b)                                                      \
  (a).tv_sec == (b).tv_sec ? (a).tv_nsec < (b).tv_nsec : (a).tv_sec < (b).tv_sec

#define timespec_time(t) (t).tv_sec *NANOS + (t).tv_nsec

#define timespec_of(ns)                                                        \
  (struct timespec) { .tv_sec = (ns) / NANOS, .tv_nsec = (ns) % NANOS }

static int ssm_sem_fd;
atomic_size_t rb_r;
atomic_size_t rb_w;
ssm_value_t ssm_stdin;
pthread_t ssm_stdin_tid;
pthread_attr_t ssm_stdin_attr;
ssm_value_t ssm_stdout;

typedef struct {
  ssm_act_t act;
  ssm_trigger_t trigger1;
} main_act_t;

typedef struct {
  ssm_act_t act;
  ssm_trigger_t trigger;
} stdout_handler_act_t;

ssm_stepf_t step_main, step_stdout_handler;

ssm_act_t *enter_stdout_handler(ssm_act_t *parent, ssm_priority_t priority,
                                ssm_depth_t depth) {
  stdout_handler_act_t *cont =
      container_of(ssm_enter(sizeof(stdout_handler_act_t), step_stdout_handler,
                             parent, priority, depth),
                   stdout_handler_act_t, act);
  cont->trigger.act = &cont->act;
  return &cont->act;
}

void step_stdout_handler(ssm_act_t *act) {
  stdout_handler_act_t *cont = container_of(act, stdout_handler_act_t, act);

  switch (act->pc) {
  case 0:
    ssm_sensitize(ssm_to_sv(ssm_stdout), &cont->trigger);
    act->pc = 1;
    return;
  case 1:;
    char c = ssm_unmarshal(ssm_deref(ssm_stdout));
    putchar(c);
    return;
  }
  ssm_leave(&cont->act, sizeof(stdout_handler_act_t));
}

ssm_act_t *enter_main(ssm_act_t *parent, ssm_priority_t priority,
                      ssm_depth_t depth) {

  main_act_t *cont = container_of(
      ssm_enter(sizeof(main_act_t), step_main, parent, priority, depth),
      main_act_t, act);
  cont->trigger1.act = &cont->act;
  return &cont->act;
}

void step_main(ssm_act_t *act) {
  main_act_t *cont = container_of(act, main_act_t, act);
  switch (act->pc) {
  case 0:
    for (;;) {
      ssm_sensitize(ssm_to_sv(ssm_stdin), &cont->trigger1);
      act->pc = 1;
      return;
    case 1:;
      ssm_desensitize(&cont->trigger1);
      char c = ssm_unmarshal(ssm_deref(ssm_stdin));

      ssm_later(ssm_to_sv(ssm_stdout), ssm_now() + (NANOS / 2),
                ssm_marshal(c >= 'a' && c <= 'z' ? c + 'A' - 'a' : c));

      ssm_sensitize(ssm_to_sv(ssm_stdout), &cont->trigger1);
      act->pc = 2;
      return;
    case 2:;
      ssm_later(ssm_to_sv(ssm_stdout), ssm_now() + (NANOS / 1000),
                ssm_marshal('\n'));
      act->pc = 3;
      return;
    case 3:;
      ssm_desensitize(&cont->trigger1);
      // printf("%s: ready to accept more input (missed latest: %c)\n",
      //        __FUNCTION__, ssm_unmarshal(ssm_deref(ssm_stdin)));
    }
  }
  ssm_leave(act, sizeof(main_act_t));
}

void *ssm_stdin_handler(void *sv) {
  for (;;) {
    int c = getchar();
    if (c == EOF) {
      return NULL;
    }
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    uint64_t t = timespec_time(now);

    size_t w, r;
    w = atomic_load(&rb_w);
    r = atomic_load(&rb_r);

    if (ssm_input_write_ready(r, w)) {
      struct ssm_input *packet = ssm_input_get(w);
      packet->sv = sv;
      packet->payload = ssm_marshal(c);
      packet->time.raw_time64 = t;
      atomic_store(&rb_w, w + 1);
      eventfd_write(ssm_sem_fd, 1);
    } else {
      fprintf(stderr, "Dropped input packet: %c (r=%ld, w=%ld)\n", c, r, w);
    }
  }
}

void ssm_program_init(void) {
  ssm_stdin = ssm_new(SSM_BUILTIN, SSM_SV_T);
  ssm_sv_init(ssm_stdin, ssm_marshal(0));

  ssm_stdout = ssm_new(SSM_BUILTIN, SSM_SV_T);
  ssm_sv_init(ssm_stdout, ssm_marshal(0));

  ssm_activate(enter_stdout_handler(&ssm_top_parent, SSM_ROOT_PRIORITY,
                                    SSM_ROOT_DEPTH - 1));
  ssm_activate(enter_main(&ssm_top_parent,
                          SSM_ROOT_PRIORITY + (1 << (SSM_ROOT_DEPTH - 1)),
                          SSM_ROOT_DEPTH - 1));

  pthread_create(&ssm_stdin_tid, &ssm_stdin_attr, ssm_stdin_handler,
                 ssm_to_sv(ssm_stdin));
}

void ssm_program_exit(void) {
  printf("DBG: joining stdin handler\n");
  pthread_join(ssm_stdin_tid, NULL);
}

#define MAX_PAGES 2048
static void *pages[MAX_PAGES];
static size_t allocated_pages = 0;
static void *alloc_page(void) {
  if (allocated_pages >= MAX_PAGES) {
    SSM_THROW(SSM_EXHAUSTED_MEMORY);
    exit(3);
  }
  void *m = pages[allocated_pages++] = malloc(SSM_MEM_PAGE_SIZE);
  memset(m, 0, SSM_MEM_PAGE_SIZE);
  return m;
}

static void *alloc_mem(size_t size) { return malloc(size); }

static void free_mem(void *mem, size_t size) { free(mem); }

int main(void) {
  ssm_mem_init(alloc_page, alloc_mem, free_mem);
  ssm_sem_fd = eventfd(0, EFD_NONBLOCK);

  struct timespec init_time;
  clock_gettime(CLOCK_MONOTONIC, &init_time);
  ssm_set_now(timespec_time(init_time));
  ssm_program_init();

  size_t scaled = 0;
  for (;;) {
    size_t r, w;

    eventfd_t e;
    eventfd_read(ssm_sem_fd, &e);

    r = atomic_load(&rb_r);
    w = atomic_load(&rb_w);

    for (; scaled < w; scaled++)
      ssm_input_get(scaled)->time =
          ssm_raw_time64_scale(ssm_input_get(scaled)->time, 1);

    do {
      r = ssm_input_consume(r, w);
      atomic_store(&rb_r, r);
      ssm_tick();
    } while (ssm_input_read_ready(r, w));
    fflush(stdout);

    fd_set in_fds;
    FD_SET(ssm_sem_fd, &in_fds);

    ssm_time_t next = ssm_next_event_time();
    if (next == SSM_NEVER) {
      // printf("%s: sleeping indefinitely\n", __FUNCTION__);
      pselect(ssm_sem_fd + 1, &in_fds, NULL, NULL, NULL, NULL);
    } else {
      struct timespec next_time = timespec_of(next);
      struct timespec wall_time;
      clock_gettime(CLOCK_MONOTONIC, &wall_time);
      // printf("%s: next time is %ld (currently (%ld)\n", __FUNCTION__,
      //        timespec_time(next_time), timespec_time(wall_time));
      if (timespec_lt(wall_time, next_time)) {
        struct timespec sleep_time = timespec_diff(next_time, wall_time);
        // printf("%s: sleeping for %ld\n", __FUNCTION__,
        //        timespec_time(sleep_time));
        pselect(ssm_sem_fd + 1, &in_fds, NULL, NULL, &sleep_time, NULL);
      }
    }
  }

  ssm_program_exit();

  for (size_t p = 0; p < allocated_pages; p++)
    free(pages[p]);

  return 0;
}
void ssm_throw(enum ssm_error reason, const char *file, int line,
               const char *func) {
  fprintf(stderr, "SSM error at %s:%s:%d: reason: %d\n", file, func, line,
          reason);
  void *array[64];
  int size;
  fprintf(stderr, "Backtrace:\n\n");
  size = backtrace(array, 64);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}
