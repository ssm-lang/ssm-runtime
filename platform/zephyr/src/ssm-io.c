#include <platform/internal/ssm-io.h>
#include <platform/ssm-timer.h>
#include <platform/ssm-log.h>
#include <platform/ssm-prof.h>

#include <devicetree.h>
#include <drivers/gpio.h>

/**** INPUT *******************************************************************/

SSM_RB_DECLARE(ssm_input_packet_t, ssm_input_buffer, 12);

typedef struct {
  struct gpio_dt_spec spec;
  struct gpio_callback cb;
  ssm_sv_t *sv;
} ssm_input_t;

static void input_event_handler(const struct device *port,
                                struct gpio_callback *cb,
                                gpio_port_pins_t pins) {

  uint32_t hi0, lo, hi1;
  uint32_t key;
  ssm_input_packet_t *input_packet;

  SSM_PROF(SSM_PROF_INPUT_BEFORE_READTIME);

  key = irq_lock();

  ssm_timer_read_to(&hi0, &lo, &hi1);

  SSM_PROF(SSM_PROF_INPUT_BEFORE_ALLOC);

  if ((input_packet = ssm_rb_writer_alloc(ssm_input_buffer))) {
    SSM_PROF(SSM_PROF_INPUT_BEFORE_COMMIT);

    input_packet->hi0 = hi0;
    input_packet->lo = lo;
    input_packet->hi1 = hi1;
    input_packet->sv = container_of(cb, ssm_input_t, cb)->sv;
    input_packet->payload =
        gpio_pin_get(port, container_of(cb, ssm_input_t, cb)->spec.pin);

    compiler_barrier();

    ssm_rb_writer_commit(ssm_input_buffer);

    SSM_PROF(SSM_PROF_INPUT_BEFORE_WAKE);

    ssm_sem_post(&ssm_tick_sem);

  } else {
    dropped_packets++;
  }

  irq_unlock(key);
  SSM_PROF(SSM_PROF_INPUT_BEFORE_LEAVE);
}

typedef int (*ssm_input_setup_t)(ssm_sv_t *sv, ssm_input_t *in);

static inline int initialize_input_gpio_edge(ssm_sv_t *sv, ssm_input_t *in, gpio_flags_t flags) {
  int err;

  in->sv = sv;

  if ((err = gpio_pin_configure(in->spec.port, in->spec.pin,
                                GPIO_INPUT | in->spec.dt_flags)))
    return err;

  if ((err = gpio_pin_interrupt_configure(in->spec.port, in->spec.pin, flags)))
    return err;

  gpio_init_callback(&in->cb, input_event_handler, BIT(in->spec.pin));

  err = gpio_add_callback(in->spec.port, &in->cb);
  if (err)
    return err;

  return 0;
}

__attribute__((unused))
static int initialize_input_gpio_bool(ssm_sv_t *sv, ssm_input_t *in) {
  return initialize_input_gpio_edge(sv, in, GPIO_INT_EDGE_BOTH);
}

static int initialize_input_gpio_event(ssm_sv_t *sv, ssm_input_t *in) {
  return initialize_input_gpio_edge(sv, in, GPIO_INT_EDGE_TO_ACTIVE);
}

#if !DT_NODE_HAS_STATUS(DT_ALIAS(sw0), okay)
#error "sw0 device alias not defined"
#endif
#ifndef PLATFORM_BOARD_disco_f407vg
#if !DT_NODE_HAS_STATUS(DT_ALIAS(sw1), okay)
#error "sw1 device alias not defined"
#endif
#if !DT_NODE_HAS_STATUS(DT_ALIAS(sw2), okay)
#error "sw2 device alias not defined"
#endif
#if !DT_NODE_HAS_STATUS(DT_ALIAS(sw3), okay)
#error "sw3 device alias not defined"
#endif
#endif

struct {
  ssm_input_t in;
  ssm_input_setup_t init;
} static_inputs[] = {
    [0] = {.in = {.spec = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios)},
           .init = initialize_input_gpio_event},
#ifndef PLATFORM_BOARD_disco_f407vg
    [1] = {.in = {.spec = GPIO_DT_SPEC_GET(DT_ALIAS(sw1), gpios)},
           .init = initialize_input_gpio_event},
    [2] = {.in = {.spec = GPIO_DT_SPEC_GET(DT_ALIAS(sw2), gpios)},
           .init = initialize_input_gpio_event},
    [3] = {.in = {.spec = GPIO_DT_SPEC_GET(DT_ALIAS(sw3), gpios)},
           .init = initialize_input_gpio_event},
#endif
};

int bind_static_input_device(ssm_sv_t *sv, int fd) {
  return static_inputs[fd].init(sv, &static_inputs[fd].in);
}

typedef struct {
  /** Activation record fields */
  ssm_act_t act;
  ssm_trigger_t trigger;

  /** Statically initialized for static output devices */
  struct gpio_dt_spec spec;

  /** Bound at runtime */
  ssm_sv_t *sv;
} ssm_output_t;

/**** OUTPUT ******************************************************************/

static void step_out_gpio_bool_handler(ssm_act_t *actg) {
  ssm_output_t *out = container_of(actg, ssm_output_t, act);
  ssm_bool_t *sv = container_of(out->sv, ssm_bool_t, sv);

  switch (actg->pc) {
  case 0:
    out->trigger.act = actg;
    ssm_sensitize(&sv->sv, &out->trigger);
    actg->pc = 1;
    return;

  case 1:
    gpio_pin_set(out->spec.port, out->spec.pin, sv->value);
    return;
  }
  // Unreachable
  // ssm_desensitize(&out->trigger);
}

static int initialize_output_gpio_bool(ssm_act_t *parent,
                                       ssm_priority_t priority,
                                       ssm_depth_t depth, ssm_sv_t *sv,
                                       ssm_output_t *out) {
  int err;

  out->sv = sv;

  // TODO: Push this back into ssm library
  out->act = (ssm_act_t){
      .step = step_out_gpio_bool_handler,
      .caller = parent,
      .pc = 0,
      .children = 0,
      .priority = priority,
      .depth = depth,
      .scheduled = false,
  };

  if ((err = gpio_pin_configure(out->spec.port, out->spec.pin,
                                GPIO_OUTPUT_ACTIVE | out->spec.dt_flags)))
    return err;

  if ((err = gpio_pin_set(out->spec.port, out->spec.pin,
                          container_of(out->sv, ssm_bool_t, sv)->value)))
    return err;

  return 0;
}

#if !DT_NODE_HAS_STATUS(DT_ALIAS(led0), okay)
#error "led0 device alias not defined"
#endif
#ifndef PLATFORM_BOARD_disco_f407vg
#if !DT_NODE_HAS_STATUS(DT_ALIAS(led1), okay)
#error "led1 device alias not defined"
#endif
#if !DT_NODE_HAS_STATUS(DT_ALIAS(led2), okay)
#error "led2 device alias not defined"
#endif
#if !DT_NODE_HAS_STATUS(DT_ALIAS(led3), okay)
#error "led3 device alias not defined"
#endif
#endif

typedef int (*ssm_output_setup_t)(ssm_act_t *parent, ssm_priority_t priority,
                                  ssm_depth_t depth, ssm_sv_t *sv,
                                  ssm_output_t *out);

static struct {
  ssm_output_t out;
  ssm_output_setup_t init;
} static_outputs[] = {
    [0] = {.out = {.spec = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios)},
           .init = initialize_output_gpio_bool},
#ifndef PLATFORM_BOARD_disco_f407vg
    [1] = {.out = {.spec = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios)},
           .init = initialize_output_gpio_bool},
    [2] = {.out = {.spec = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios)},
           .init = initialize_output_gpio_bool},
    [3] = {.out = {.spec = GPIO_DT_SPEC_GET(DT_ALIAS(led3), gpios)},
           .init = initialize_output_gpio_bool},
#endif
};

ssm_act_t *bind_static_output_device(ssm_act_t *parent, ssm_priority_t priority,
                                     ssm_depth_t depth, ssm_sv_t *sv, int fd) {
  static_outputs[fd].init(parent, priority, depth, sv, &static_outputs[fd].out);
  return &static_outputs[fd].out.act;
}
