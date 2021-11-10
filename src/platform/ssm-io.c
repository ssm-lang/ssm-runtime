/**
 * A sketch of what an I/O subsystem for SSM could look like, where regular SSM
 * variables are "bound" to input and output devices through functions provided
 * here. Specialized to work with the nRF52840-DK's LEDs and buttons.
 */
#include <platform/internal/ssm-io.h>
#include <platform/internal/ssm-sem.h>
#include <platform/ssm-platform.h>
#include <platform/ssm-timer.h>
#include <platform/ssm-log.h>
#include <platform/ssm-prof.h>
#include <platform/ssm-sleep.h>

#include <devicetree.h>
#include <sys/atomic.h>

SSM_LOG_NAME(ssm_io);

SSM_RB_DEFINE(ssm_input_packet_t, ssm_input_buffer, 12);
SSM_SEM_DEFINE(ssm_tick_sem);
uint32_t dropped_packets;

void ssm_tick_log(void) {
  for (;;) {
    SSM_MSLEEP(SSM_TICK_LOG_PERIOD_MS);
    if (dropped_packets) {
      LOG_WRN("Dropped %u packets\r\n", dropped_packets);
      dropped_packets = 0;
    }
  }
}

static void send_timeout_event(uint64_t ticks, void *user_data) {
  ssm_sem_post(&ssm_tick_sem);
}

void ssm_tick_loop(void) {
  SSM_PROF_INIT();

  SSM_PROF(SSM_PROF_MAIN_SETUP);
  ssm_timer_start();

  ssm_program_initialize();

  // TODO: we are assuming no input events at time 0.
  ssm_tick();

  for (;;) {
    SSM_PROF(SSM_PROF_MAIN_BEFORE_LOOP_CONTINUE);
    ssm_input_packet_t *input_packet;
    ssm_time_t next_time, wall_time;

    wall_time = ssm_timer_read();
    next_time = ssm_next_event_time();

    compiler_barrier(); // Must read timer before reading from input queue

    if ((input_packet = ssm_rb_reader_claim(ssm_input_buffer))) {
      ssm_time_t packet_time = ssm_input_packet_time(input_packet);

      if (packet_time <= next_time) {
        SSM_PROF(SSM_PROF_MAIN_BEFORE_INPUT_CONSUME);
        // We are ready to process that input event.
        // Schedule it and consume input packet from ring buffer
        ssm_schedule(input_packet->sv, packet_time);

        ssm_rb_reader_free(ssm_input_buffer);
        // Check for more input or call tick.
        continue;
      }
    }
    if (next_time <= wall_time) {
      // Ready to tick on internal event
      SSM_PROF(SSM_PROF_MAIN_BEFORE_TICK);
      ssm_tick();
    } else {
      // Nothing to do; sleep
      if (next_time != SSM_NEVER) {
        SSM_PROF(SSM_PROF_MAIN_BEFORE_SLEEP_ALARM);
        // Set alarm
        int err = ssm_timer_set_alarm(next_time, send_timeout_event, NULL);

        // Handle errors
        switch (err) {
        case 0:
        case -ETIME:
          // set_alarm successful (or expired)
          break;
        case -EBUSY:
          SSM_DEBUG_ASSERT(-EBUSY, "set_alarm failed: already set\r\n");
        case -ENOTSUP:
          SSM_DEBUG_ASSERT(-ENOTSUP, "set_alarm failed: not supported\r\n");
        case -EINVAL:
          SSM_DEBUG_ASSERT(-EINVAL, "set_alarm failed: invalid settings\r\n");
        default:
          SSM_DEBUG_ASSERT(err, "set_alarm failed for unknown reasons\r\n");
        }
      }
      SSM_PROF(SSM_PROF_MAIN_BEFORE_SLEEP_BLOCK);
      ssm_sem_wait(&ssm_tick_sem);
      SSM_PROF(SSM_PROF_MAIN_BEFORE_WAKE_RESET);

      // Cancel any potential pending alarm if it hasn't gone off yet.
      ssm_timer_cancel_alarm();

      // It's possible that the alarm went off before we cancelled it; make
      // sure that its sem_give doesn't cause premature wake-up later on.
      ssm_sem_reset(&ssm_tick_sem);
    }
  }
}
