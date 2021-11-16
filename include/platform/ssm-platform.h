#ifndef _SSM_PLATFORM_H
#define _SSM_PLATFORM_H

#include <ssm.h>

extern int ssm_program_initialize(void);

extern ssm_act_t *bind_static_output_device(ssm_act_t *parent,
                                            ssm_priority_t priority,
                                            ssm_depth_t depth, ssm_sv_t *sv,
                                            int fd);
extern int bind_static_input_device(ssm_sv_t *sv, int fd);
extern void ssm_tick_log(void);
extern void ssm_tick_loop(void);

#endif /* _SSM_PLATFORM_H */
