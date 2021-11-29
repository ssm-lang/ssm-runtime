#ifndef _SSM_TYPES_H
#define _SSM_TYPES_H

#include <ssm.h>

typedef int8_t   i8;   /**< 8-bit Signed Integer */
typedef int16_t  i16;  /**< 16-bit Signed Integer */
typedef int32_t  i32;  /**< 32-bit Signed Integer */
typedef int64_t  i64;  /**< 64-bit Signed Integer */
typedef uint8_t  u8;   /**< 8-bit Unsigned Integer */
typedef uint16_t u16;  /**< 16-bit Unsigned Integer */
typedef uint32_t u32;  /**< 32-bit Unsigned Integer */
typedef uint64_t u64;  /**< 64-bit Unsigned Integer */

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

/* typedef struct { ssm_sv_t sv; } ssm_event_t; */
/* #define ssm_later_event(var, then) ssm_schedule(&(var)->sv, (then)) */
/* extern void ssm_assign_event(ssm_event_t *var, ssm_priority_t prio); */
/* extern void ssm_initialize_event(ssm_event_t *); */

/* #define SSM_DECLARE_SV_SCALAR(payload_t)                                       \ */
/*   typedef struct {                                                             \ */
/*     ssm_sv_t sv;                                                          \ */
/*     payload_t value;       /1* Current value *1/                                 \ */
/*     payload_t later_value; /1* Buffered value *1/                                \ */
/*   } ssm_##payload_t##_t;                                                       \ */
/*   void ssm_assign_##payload_t(ssm_##payload_t##_t *sv, ssm_priority_t prio,    \ */
/*                           const payload_t value);                              \ */
/*   void ssm_later_##payload_t(ssm_##payload_t##_t *sv, ssm_time_t then,         \ */
/*                          const payload_t value);                               \ */
/*   void ssm_initialize_##payload_t(ssm_##payload_t##_t *v); */

/* #define SSM_DEFINE_SV_SCALAR(payload_t)                                        \ */
/*   static void ssm_update_##payload_t(ssm_sv_t *sv) {                      \ */
/*     ssm_##payload_t##_t *v = container_of(sv, ssm_##payload_t##_t, sv);        \ */
/*     v->value = v->later_value;                                                 \ */
/*   }                                                                            \ */
/*   void ssm_assign_##payload_t(ssm_##payload_t##_t *v, ssm_priority_t prio,     \ */
/*                               const payload_t value) {                         \ */
/*     v->value = value;					                       \ */
/*     v->sv.last_updated = ssm_now();			  		       \ */
/*     ssm_trigger(&v->sv, prio);                                                 \ */
/*   }                                                                            \ */
/*   void ssm_later_##payload_t(ssm_##payload_t##_t *v, ssm_time_t then,          \ */
/*                          const payload_t value) {                              \ */
/*     v->later_value = value;                                                    \ */
/*     ssm_schedule(&v->sv, then);					               \ */
/*   }		 	 						       \ */
/*   void ssm_initialize_##payload_t(ssm_##payload_t##_t *v) {                    \ */
/*     ssm_initialize(&v->sv, ssm_update_##payload_t);	     	               \ */
/*   } */

/** \struct ssm_bool_t
    Scheduled Boolean variable */
/** \struct ssm_i8_t
    Scheduled 8-bit Signed Integer variable */
/** \struct ssm_i16_t
    Scheduled 16-bit Signed Integer variable */
/** \struct ssm_i32_t
    Scheduled 32-bit Signed Integer variable */
/** \struct ssm_i64_t
    Scheduled 64-bit Signed Integer variable */
/** \struct ssm_u8_t
    Scheduled 8-bit Unsigned Integer variable */
/** \struct ssm_u16_t
    Scheduled 16-bit Unsigned Integer variable */
/** \struct ssm_u32_t
    Scheduled 32-bit Unsigned Integer variable */
/** \struct ssm_u64_t
    Scheduled 64-bit Unsigned Integer variable */

/* SSM_DECLARE_SV_SCALAR(bool) */
/* SSM_DECLARE_SV_SCALAR(i8) */
/* SSM_DECLARE_SV_SCALAR(i16) */
/* SSM_DECLARE_SV_SCALAR(i32) */
/* SSM_DECLARE_SV_SCALAR(i64) */
/* SSM_DECLARE_SV_SCALAR(u8) */
/* SSM_DECLARE_SV_SCALAR(u16) */
/* SSM_DECLARE_SV_SCALAR(u32) */
/* SSM_DECLARE_SV_SCALAR(u64) */
#endif /* _SSM_TYPES_H */
