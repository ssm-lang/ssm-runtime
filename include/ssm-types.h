#ifndef _SSM_TYPES_H
#define _SSM_TYPES_H

#include <ssm.h>

typedef ssm_value_t i8;  /**< 8-bit Signed Integer */
typedef ssm_value_t i16; /**< 16-bit Signed Integer */
typedef ssm_value_t i32; /**< 32-bit Signed Integer */
typedef ssm_value_t i64; /**< 64-bit Signed Integer */
typedef ssm_value_t u8;  /**< 8-bit Unsigned Integer */
typedef ssm_value_t u16; /**< 16-bit Unsigned Integer */
typedef ssm_value_t u32; /**< 32-bit Unsigned Integer */
typedef ssm_value_t u64; /**< 64-bit Unsigned Integer */

typedef ssm_sv_t ssm_event_t;
typedef ssm_sv_t ssm_bool_t;
typedef ssm_sv_t ssm_i8_t;
typedef ssm_sv_t ssm_i16_t;
typedef ssm_sv_t ssm_i32_t;
typedef ssm_sv_t ssm_i64_t;
typedef ssm_sv_t ssm_u8_t;
typedef ssm_sv_t ssm_u16_t;
typedef ssm_sv_t ssm_u32_t;
typedef ssm_sv_t ssm_u64_t;

#define EVENT_VALUE (ssm_marshal(0))

#endif /* _SSM_TYPES_H */
