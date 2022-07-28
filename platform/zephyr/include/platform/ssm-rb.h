#ifndef _PLATFORM_SSM_RB_H
#define _PLATFORM_SSM_RB_H

#include <sys/atomic.h>

typedef atomic_t ssm_rb_idx_t;
#define SSM_RB_IDX_INIT(val) ATOMIC_INIT(val)
#define ssm_rb_idx_get(p_idx) atomic_get(p_idx)
#define ssm_rb_idx_inc(p_idx) atomic_inc(p_idx)

#endif
