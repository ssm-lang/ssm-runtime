#ifndef _PLATFORM_SSM_MSGQ_H
#define _PLATFORM_SSM_MSGQ_H

#include <kernel.h>

#define SSM_MSGQ_DEFINE(q, msg_sz, max_msgs, align) \
  K_MSGQ_DEFINE(q, msg_sz, max_msgs, align)

#define SSM_MSGQ_BLOCK K_FOREVER
#define SSM_MSGQ_NONBLOCK K_NO_WAIT

#define ssm_msgq_put(q, msg, fl) \
  k_msgq_put(q, msg, fl)

#define ssm_msgq_get(q, msg, fl) \
  k_msgq_get(q, msg, fl)

#endif /* ifndef _PLATFORM_SSM_MSGQ_H */
