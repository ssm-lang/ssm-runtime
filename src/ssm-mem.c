/** @file ssm-mem.c
 *  @brief SSM runtime memory management.
 *
 *  @author John Hui (j-hui)
 *  @author Daniel Scanteianu (Scanteianu)
 */
#include <assert.h>
#include <ssm-internal.h>
#include <ssm.h>
#include <stdio.h>
#include <stdlib.h>

uint8_t ssm_page_init[SSM_MEM_PAGE_SIZE];

typedef union block {
  union block *free_list_next;
  ssm_word_t free_list_next_idx;
  uint8_t block_buf[1];
} block_t;

#define BLOCKS_PER_PAGE (SSM_MEM_PAGE_SIZE / sizeof(block_t))

#define block_is_index(b) ((b)->free_list_next_idx - 1 < BLOCKS_PER_PAGE - 1)

#define block_page_of_ptr(b)                                                   \
  ((block_t *)((ssm_word_t)b->block_buf & ~(SSM_MEM_PAGE_SIZE - 1)))

struct mem_pool {
  block_t *free_list_head;
};

struct mem_pool mem_pools[SSM_MEM_POOL_COUNT];

static void *(*alloc_page)(void);
static void *(*alloc_mem)(size_t size);
static void (*free_mem)(void *mem, size_t size);

static inline size_t find_pool_size(size_t size) {
  for (size_t pool_idx = 0; pool_idx < SSM_MEM_POOL_COUNT; pool_idx++)
    if (size < SSM_MEM_POOL_SIZE(pool_idx))
      return pool_idx;
  return SSM_MEM_POOL_COUNT;
}

void ssm_mem_init(void *(*alloc_page_handler)(void),
                  void *(*alloc_mem_handler)(size_t),
                  void (*free_mem_handler)(void *, size_t)) {
  alloc_page = alloc_page_handler;
  alloc_mem = alloc_mem_handler;
  free_mem = free_mem_handler;

  /** First pass: initialize this list with pointers. */
  block_t *page_fractal = (block_t *)ssm_page_init;
  block_t *prev = page_fractal[0].free_list_next =
      page_fractal + BLOCKS_PER_PAGE / 2;

  for (size_t level = BLOCKS_PER_PAGE / 4; level > 0; level /= 2) {
    prev = prev->free_list_next = page_fractal + level;
    while (prev + level * 2 < page_fractal + BLOCKS_PER_PAGE)
      prev = prev->free_list_next = prev + level * 2;
  }

  prev->free_list_next = NULL;

  SSM_ASSERT(prev - page_fractal == BLOCKS_PER_PAGE - 1);

  /** Second pass: convert pointers to relative indices. */
  for (size_t p = 0; p < BLOCKS_PER_PAGE - 1; p++)
    page_fractal[p].free_list_next_idx =
        page_fractal[p].free_list_next - page_fractal;

  /* for (size_t p = 0; p < BLOCKS_PER_PAGE - 1; p++) */
  /*   printf("%ld => %ld\n", p, page_fractal[p].free_list_next_idx >> 1); */

  /* printf("%ld => %ld\n", BLOCKS_PER_PAGE - 1, */
  /*        page_fractal[BLOCKS_PER_PAGE - 1].free_list_next_idx); */
  /* exit(1); */
}

void *ssm_mem_alloc(size_t size) {
  size_t p = find_pool_size(size);
  if (p >= SSM_MEM_POOL_COUNT)
    return alloc_mem(size);
  /* static uint8_t *b; */

  struct mem_pool *pool = &mem_pools[p];

  if (pool->free_list_head == NULL) {
    printf("allocating new page\n");
    pool->free_list_head = alloc_page();

    SSM_ASSERT((ssm_word_t)pool->free_list_head > BLOCKS_PER_PAGE);
    /* SSM_ASSERT(((ssm_word_t)pool->free_list_head & (SSM_MEM_PAGE_SIZE - 1))
     * == */
    /* 0); */

    /* b = mem_pools[pool].free_list_head->block_buf; */

    /* printf("Pool starts here: %p\n", (void *)b); */

    /* size_t last_block = BLOCKS_PER_PAGE - (SSM_MEM_PAGE_SIZE /
     * SSM_MEM_POOL_SIZE(pool)); */

    size_t last_block = 504;

    /* printf("PAGE_SIZE: %dB\n", SSM_MEM_PAGE_SIZE); */
    /* printf("POOL_SIZE: %dB\n", SSM_MEM_POOL_SIZE(pool)); */
    /* printf("BLOCK_PER_CHUNK: %d\n", SSM_MEM_PAGE_SIZE /
     * SSM_MEM_POOL_SIZE(pool)); */
    /* printf("CHUNKS_PER_PAGE: %ld\n", BLOCKS_PER_PAGE); */
    /* printf("last_block: %ld\n", last_block); */

    pool->free_list_head[last_block].free_list_next = NULL;
    /* printf("OK\n"); */
  }

  /* printf("current head: %p %p\n", (void *)mem_pools[pool].free_list_head, */
  /*        (void *)mem_pools[pool].free_list_head->block_buf); */
  /* printf("current head: %ld\n", */
  /*        (b - (unsigned char *)mem_pools[pool].free_list_head) / */
  /*            sizeof(block_t)); */

  void *buf = pool->free_list_head->block_buf;
  printf("%p\n", (void *)pool->free_list_head);
  printf("  %s\n", block_is_index(pool->free_list_head) ? "is index" : "not index");
  printf("%lx\n", pool->free_list_head->free_list_next_idx);

  if (block_is_index(pool->free_list_head)) {
    pool->free_list_head = block_page_of_ptr(pool->free_list_head) +
                                   pool->free_list_head->free_list_next_idx;
  printf("set as next: %p\n", (void *)pool->free_list_head);
  } else {
    pool->free_list_head = pool->free_list_head->free_list_next;
  }
  /* pool->free_list_head = block_is_index(pool->free_list_head) */
  /*                            ? block_page_of_ptr(pool->free_list_head) + */
  /*                                  pool->free_list_head->free_list_next_idx */
  /*                            : pool->free_list_head->free_list_next; */
  return buf;
}

void ssm_mem_free(void *m, size_t size) {
  size_t pool = find_pool_size(size);
  if (pool >= SSM_MEM_POOL_COUNT) {
    free_mem(m, size);
    return;
  }

  block_t *new_head = m;
  new_head->free_list_next = mem_pools[pool].free_list_head;
  mem_pools[pool].free_list_head = new_head;
}

/** TODO: doc */
static inline void drop_children(struct ssm_mm *mm) {
  if (!ssm_mm_is_builtin(mm)) {
    struct ssm_object *obj = container_of(mm, struct ssm_object, mm);
    for (size_t i = 0; i < mm->val_count; i++)
      if (ssm_on_heap(obj->payload[i]))
        ssm_drop(obj->payload[i].heap_ptr);
  } else {
    switch (mm->tag) {
    case SSM_SV_T: {
      ssm_sv_t *obj = container_of(mm, ssm_sv_t, mm);
      if (ssm_on_heap(obj->value)) {
        ssm_drop(obj->value.heap_ptr);
      }
      if (obj->later_time != SSM_NEVER && ssm_on_heap(obj->later_value)) {
        ssm_drop(obj->later_value.heap_ptr);
      }
    } break;
    }
  }
}

struct ssm_object *ssm_new(uint8_t val_count, uint8_t tag) {
  SSM_ASSERT(val_count > 0);
  struct ssm_mm *mm = ssm_mem_alloc(SSM_OBJ_SIZE(val_count));
  mm->val_count = val_count;
  mm->tag = tag;
  mm->ref_count = 1;
  return container_of(mm, struct ssm_object, mm);
}

void ssm_dup(struct ssm_mm *mm) { ++mm->ref_count; }

void ssm_drop(struct ssm_mm *mm) {
  if (--mm->ref_count == 0) {
    drop_children(mm);
    ssm_mem_free(mm, ssm_mm_is_builtin(mm) ? SSM_BUILTIN_SIZE(mm->tag)
                                           : SSM_OBJ_SIZE(mm->val_count));
  }
}

struct ssm_mm *ssm_reuse(struct ssm_mm *mm) {
  SSM_ASSERT(0);
  return NULL;
  /* if (--mm->ref_count == 0) { */
  /*   drop_children(mm); */
  /*   return mm; */
  /* } else { */
  /*   if (ssm_mm_is_builtin(mm)) { */
  /*     return ssm_new_builtin(mm->tag); */
  /*   } else { */
  /*     return &ssm_new(mm->val_count, mm->tag)->mm; */
  /*   } */
  /* } */
}
