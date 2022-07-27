#ifndef _PLATFORM_SPECIFIC_SSM_IO_H
#define _PLATFORM_SPECIFIC_SSM_IO_H

#include <drivers/gpio.h>

#include <ssm.h>
#include <board/ssm-io.h>

#ifndef SSM_OUT_COUNT
#error "SSM_OUT_COUNT not defined for board"
#endif

typedef struct {
  struct gpio_dt_spec spec;
  struct gpio_callback cb;
  ssm_sv_t *sv;
} ssm_input_t;

typedef struct {
  /** Activation record fields */
  ssm_act_t act;
  ssm_trigger_t trigger;

  /** Statically initialized for static output devices */
  struct gpio_dt_spec spec;

  /** Bound at runtime */
  ssm_sv_t *sv;
} ssm_output_t;

extern int initialize_input_gpio_event(ssm_sv_t *sv, ssm_input_t *in);
extern int initialize_output_gpio_bool(ssm_act_t *parent,
                                       ssm_priority_t priority,
                                       ssm_depth_t depth, ssm_sv_t *sv,
                                       ssm_output_t *out);

extern ssm_input_t static_inputs[SSM_IN_COUNT];
extern ssm_output_t static_outputs[SSM_OUT_COUNT];

// These should fail to compile if fd isn't specified in this header
#define ssm_bind_static_input_device(sv, fd) \
  (SSM_IN_INIT_##fd(sv, &static_inputs[fd]))
#define ssm_bind_static_output_device(parent, priority, depth, sv, fd)  \
  (SSM_OUT_INIT_##fd(parent, priority, depth, sv, &static_outputs[fd]), \
   &static_outputs[fd].act)

#endif /* _PLATFORM_SPECIFIC_SSM_IO_H */
