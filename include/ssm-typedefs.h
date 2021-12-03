/** @file ssm-typedefs.h
 *  @brief SSM type definitions.
 *
 *  All types in SSM are now represented by #ssm_value_t. This header file
 *  constructs type names that are more self-documenting, for examples written
 *  in C.
 */
#ifndef _SSM_TYPEDEFS_H
#define _SSM_TYPEDEFS_H

#include <ssm.h>

typedef ssm_value_t i8;
typedef ssm_value_t i16;
typedef ssm_value_t i32;
typedef ssm_value_t i64;
typedef ssm_value_t u8;
typedef ssm_value_t u16;
typedef ssm_value_t u32;
typedef ssm_value_t u64;

typedef ssm_value_t ssm_event_t;
typedef ssm_value_t ssm_bool_t;
typedef ssm_value_t ssm_i8_t;
typedef ssm_value_t ssm_i16_t;
typedef ssm_value_t ssm_i32_t;
typedef ssm_value_t ssm_i64_t;
typedef ssm_value_t ssm_u8_t;
typedef ssm_value_t ssm_u16_t;
typedef ssm_value_t ssm_u32_t;
typedef ssm_value_t ssm_u64_t;

#define EVENT_VALUE (ssm_marshal(0))

#endif /* _SSM_TYPEDEFS_H */
