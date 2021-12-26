/**
 * Ring buffer implementation for SSM.
 *
 * Intended API:
 * - SSM_RB_DEFINE(type, name, size): define and initialize a ring buffer
 * - SSM_RB_DECLARE(type, name, size): declare a ring buffer
 * - ssm_rb_writer_alloc(rb):   obtain pointer to next empty buffer item if
 *                              available (NULL otherwise)
 * - ssm_rb_writer_commit(rb):  advance write head pointer, making buffer item
 *                              available to readers
 * - ssm_rb_reader_claim(rb):   obtain pointer to readable buffer item if
 *                              available (NULL otherwise)
 * - ssm_rb_reader_free(rb):    advance read head pointer, making buffer item
 *                              available for overwriting
 *
 * Intended use on writer side is as follows:
 *
 *   item_t *item = ssm_rb_writer_alloc(item_rb);
 *   if (item) {
 *     // write to item
 *     ssm_rb_writer_commit(item_rb);
 *   }
 *
 * Intended use on reader side is as follows:
 *
 *   item_t *item = ssm_rb_reader_claim(item_rb);
 *   if (item) {
 *     // read from item
 *     ssm_rb_reader_free(item_rb);
 *   }
 *
 * Readers and writers may proceed concurrently, but each must ensure at most
 * one writer/reader at a time between alloc-commit and claim-free.
 *
 * ----
 *
 * Requirements:
 * - The reader/writer methods must be entirely inlined by the compiler.
 * - The ring buffer size must be entirely inlined by the compiler.
 *
 * Nice-to-haves ([x] indicates satisifed by current impl; [ ] otherwise):
 * - [x] This ring buffer is type-polymorphic in its data.
 *   - [ ] Writer/reader methods perform proper typechecking.
 * - [x] The ring buffer can be shared between compilation units.
 *   - [x] Multiple declarations possible.
 * - [x] Generates no warnings.*
 * - [ ] The ring buffer can be embedded in other data structures.
 *
 * *Relies on non-C99 features.
 */
#ifndef _PLATFORM_SSM_RB_H
#define _PLATFORM_SSM_RB_H

#ifdef CUSTOM_SSM_RB

#else
#ifdef SSM_RB_MPSC_IMPL

#else

#include <platform-specific/ssm-rb.h>

#ifndef SSM_RB_IDX_INIT
#error "SSM_RB_IDX_INIT() not defined for platform"
#endif
#ifndef ssm_rb_idx_get
#error "ssm_rb_idx_get() not defined for platform"
#endif
#ifndef ssm_rb_idx_inc
#error "ssm_rb_idx_inc() not defined for platform"
#endif

typedef struct {
  ssm_rb_idx_t r_idx;
  ssm_rb_idx_t w_idx;
} ssm_rb_state_t;

#define SSM_RB_DECLARE(type, rb_name, size_exp)                                \
  extern ssm_rb_state_t rb_name##__state;                                      \
  extern type rb_name[1 << size_exp]

#define SSM_RB_DEFINE(type, rb_name, size_exp)                                 \
  SSM_RB_DECLARE(type, rb_name, size_exp);                                     \
  ssm_rb_state_t rb_name##__state = {.r_idx = SSM_RB_IDX_INIT(0),              \
                                     .w_idx = SSM_RB_IDX_INIT(0)};             \
  type rb_name[1 << size_exp]

#define ssm_rb_writer_alloc(rb_name)                                           \
  _ssm_rb_writer_alloc(&rb_name##__state, (void *)rb_name,                     \
                       sizeof(rb_name) / sizeof(*rb_name), sizeof(*rb_name))

#define ssm_rb_writer_commit(rb_name) ssm_rb_idx_inc(&rb_name##__state.w_idx)

#define ssm_rb_reader_claim(rb_name)                                           \
  _ssm_rb_reader_claim(&rb_name##__state, (void *)rb_name,                     \
                       sizeof(rb_name) / sizeof(*rb_name), sizeof(*rb_name))

#define ssm_rb_reader_free(rb_name) ssm_rb_idx_inc(&rb_name##__state.r_idx)

__attribute__((unused)) static inline void *
_ssm_rb_writer_alloc(const ssm_rb_state_t *state, char *buf, size_t buf_len,
                     size_t item_size) {
  uint32_t w, r;
  w = ssm_rb_idx_get(&state->w_idx);
  r = ssm_rb_idx_get(&state->r_idx);
  if ((w + 1) % buf_len == r % buf_len)
    return NULL;
  else
    return buf + item_size * (w % buf_len);
}

__attribute__((unused)) static inline void *
_ssm_rb_reader_claim(const ssm_rb_state_t *state, char *buf, size_t buf_len,
                     size_t item_size) {
  uint32_t w, r;
  w = ssm_rb_idx_get(&state->w_idx);
  r = ssm_rb_idx_get(&state->r_idx);
  if (w % buf_len == r % buf_len)
    return NULL;
  else
    return buf + item_size * (r % buf_len);
}
#endif
#endif
#endif /* ifndef _PLATFORM_SSM_RB_H */
