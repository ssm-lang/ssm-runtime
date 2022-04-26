#include "posix-common.h"

int ssm_sem_fd;
atomic_size_t rb_r;
atomic_size_t rb_w;
pthread_mutex_t rb_lk;

void ssm_program_init(void);
void ssm_program_exit(void);
char **ssm_init_args;

//   // ssm_stdin = ssm_new_sv(ssm_marshal(0));
//   // ssm_stdout = ssm_new_sv(ssm_marshal(0));
//   //
//   // ssm_activate(__enter_stdout_handler(&ssm_top_parent, SSM_ROOT_PRIORITY,
//   //                                   SSM_ROOT_DEPTH - 1));
//   // ssm_activate(enter_main(&ssm_top_parent,
//   //                         SSM_ROOT_PRIORITY + (1 << (SSM_ROOT_DEPTH - 1)),
//   //                         SSM_ROOT_DEPTH - 1));
//   //
//   // pthread_create(&ssm_stdin_tid, &ssm_stdin_attr, ssm_stdin_handler,
//   //                ssm_to_sv(ssm_stdin));
// }
//
//   // printf("DBG: joining stdin handler\n");
//   // pthread_join(ssm_stdin_tid, NULL);
// }
//

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

static inline void poll_input_queue(size_t *r, size_t *w) {
  static size_t scaled = 0;
  pthread_mutex_lock(&rb_lk);
  *r = atomic_load(&rb_r);
  *w = atomic_load(&rb_w);
  pthread_mutex_unlock(&rb_lk);
  for (; scaled < *w; scaled++)
    ssm_input_get(scaled)->time =
        ssm_raw_time64_scale(ssm_input_get(scaled)->time, 1);
}

// int ret = 0;
ssm_time_t next_time, wall_time;
size_t r, w;

int main(void) {
  ssm_mem_init(alloc_page, alloc_mem, free_mem);
  // ssm_sem_fd = eventfd(0, EFD_NONBLOCK);
  ssm_sem_fd = eventfd(0, 0);
  pthread_mutex_init(&rb_lk, NULL);

  struct timespec init_time;
  clock_gettime(CLOCK_MONOTONIC, &init_time);
  ssm_set_now(timespec_time(init_time));
  ssm_program_init();
  int ret = 0;

#if 1
  for (;;) {
    // ssm_time_t next_time, wall_time;
    struct timespec wall_spec;
    // size_t r, w;

    DBG("before getting wall time\n");
    clock_gettime(CLOCK_MONOTONIC, &wall_spec);
    wall_time = timespec_time(wall_spec);
    next_time = ssm_next_event_time();

    poll_input_queue(&r, &w);
    if (ssm_input_read_ready(r, w)) {
      if (ssm_input_get(r)->time.ssm_time <= next_time) {
        DBG("Consuming input of time: %ld\n", ssm_input_get(r)->time.ssm_time);
        r = ssm_input_consume(r, w);
        atomic_store(&rb_r, r);
        goto do_tick;
      }
    }

    if (next_time <= wall_time) {
    do_tick:
      ssm_tick();
    } else {
      fd_set in_fds;
      eventfd_t e;
      FD_SET(ssm_sem_fd, &in_fds);
      if (next_time == SSM_NEVER) {
        DBG("Sleeping indefinitely\n");
        ret = pselect(ssm_sem_fd + 1, &in_fds, NULL, NULL, NULL, NULL);
        DBG("Woke from sleeping indefinitely\n");
        ret = eventfd_read(ssm_sem_fd, &e);
      } else {
        struct timespec next_spec = timespec_of(next_time);
        struct timespec sleep_time = timespec_diff(next_spec, wall_spec);
        DBG("Sleeping\n");
        ret = pselect(ssm_sem_fd + 1, &in_fds, NULL, NULL, &sleep_time, NULL);
        DBG("Woke up from sleeping\n");
        if (ret > 0)
          ret = eventfd_read(ssm_sem_fd, &e);
        // otherwise, timed out
      }
    }
  }

#else
  for (;;) {
    size_t r, w;
    poll_input_queue(&r, &w);
  consume_loop:
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

      poll_input_queue(&r, &w);
      if (ssm_input_read_ready(r, w))
        goto consume_loop;

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
#endif

  ssm_program_exit();

  for (size_t p = 0; p < allocated_pages; p++)
    free(pages[p]);

  return ret;
}
