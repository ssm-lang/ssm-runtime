#ifndef _PLATFORM_SSM_IO_H
#define _PLATFORM_SSM_IO_H

#include <platform/ssm-sem.h>
#include <platform/ssm-rb.h>
#include <platform/ssm-timer.h>
#include <platform/ssm-prof.h>
#include <platform-specific/ssm-io.h>

// These must be macros so that compile fails on an undefined fd
#ifndef ssm_bind_static_input_device
#error "ssm_bind_static_input_device() not defined for platform"
#endif
#ifndef ssm_bind_static_output_device
#error "ssm_bind_static_output_device() not defined for platform"
#endif

#ifndef SSM_TICK_LOG_PERIOD_MS
#define SSM_TICK_LOG_PERIOD_MS 1000
#endif

typedef struct {
  ssm_sv_t *sv;
#ifndef PLATFORM_TIMER64
  uint32_t hi0;
  uint32_t lo;
  uint32_t hi1;
#else
  uint64_t time;
#endif
  uint32_t payload;
} ssm_input_packet_t;

#ifndef PLATFORM_TIMER64
#define ssm_input_packet_time(input_packet)                     \
  ssm_timer_calc(                                               \
    (input_packet)->hi0, (input_packet)->lo, (input_packet)->hi1)
#else
#define ssm_input_packet_time(input_packet)                     \
  ((input_packet)->time)
#endif

SSM_RB_DECLARE(ssm_input_packet_t, ssm_input_buffer, 12);
SSM_SEM_DECLARE(ssm_tick_sem);

extern uint32_t dropped_packets; /* perhaps use atomic for this */

__attribute__((always_inline))
static inline void ssm_input_event_handler(ssm_sv_t *sv, uint32_t payload)
{
#ifndef PLATFORM_TIMER64
  uint32_t hi0, lo, hi1;
#else
  uint64_t time;
#endif
  ssm_input_packet_t *input_packet;

  SSM_PROF(SSM_PROF_INPUT_BEFORE_READTIME);

#ifndef PLATFORM_TIMER64
  ssm_timer_read_to(&hi0, &lo, &hi1);
#else
  time = ssm_timer_read();
#endif

  SSM_PROF(SSM_PROF_INPUT_BEFORE_ALLOC);

  if ((input_packet = ssm_rb_writer_alloc(ssm_input_buffer))) {
    SSM_PROF(SSM_PROF_INPUT_BEFORE_COMMIT);

#ifndef PLATFORM_TIMER64
    input_packet->hi0 = hi0;
    input_packet->lo = lo;
    input_packet->hi1 = hi1;
#else
    input_packet->time = time;
#endif
    input_packet->sv = sv;
    input_packet->payload = payload;

    compiler_barrier();

    ssm_rb_writer_commit(ssm_input_buffer);

    SSM_PROF(SSM_PROF_INPUT_BEFORE_WAKE);

    ssm_sem_post(&ssm_tick_sem);

  } else {
    dropped_packets++;
  }

  SSM_PROF(SSM_PROF_INPUT_BEFORE_LEAVE);
}

extern int ssm_program_initialize(void);

extern void ssm_tick_log(void);
extern void ssm_tick_loop(void);

#endif /* ifndef _PLATFORM_SSM_IO_H */
