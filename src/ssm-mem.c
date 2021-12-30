/** @file ssm-mem.c
 *  @brief SSM runtime memory management and allocation.
 *
 *  @author John Hui (j-hui)
 *  @author Daniel Scanteianu (Scanteianu)
 */
#include <ssm-internal.h>

/** @brief (The beginning of) a block of memory.
 *
 *  The size of each block varies depending on which memory pool it resides in.
 *  When a block is being used (allocated), @a block_buf effectively gives
 *  a pointer to the beginning that block that can be indexed into.
 *
 *  When a block is not being used, it is maintained in a <em>free list</em>.
 *  If it is not at the head of the free list, then it is pointed to by the @a
 *  free_list_next field of some other block_t, while its own @a free_list_next
 *  points to the next block in the free list. To avoid initializing the @a
 *  free_list_next fields of "fresh" blocks, when @a free_list_next is
 *  #UNINITIALIZED_FREE_BLOCK, the next free block contiguously follows the
 *  current block in memory. The last free block points to #END_OF_FREELIST.
 */
typedef union block {
  union block *free_list_next; /**< Pointer to the next block. */
  uint8_t block_buf[1];        /**< Variable-size buffer of the block. */
} block_t;

/** @brief The number of blocks in each memory page. */
#define BLOCKS_PER_PAGE (SSM_MEM_PAGE_SIZE / sizeof(block_t))

/** @brief A "pointer" that points to the next contiguous block in memory. */
#define UNINITIALIZED_FREE_BLOCK ((block_t *)0x0)

/** @brief Sentinel value indicating the end of the free list. */
#define END_OF_FREELIST ((block_t *)0x42)

/** @brief A memory pool, which maintains a free list.
 *
 *  The size of blocks maintained by a memory pool is implicit in its position
 *  within #mem_pools.
 */
struct mem_pool {
  block_t *free_list_head; /**< Pointer to the beginning of the free list. */
};

/** @brief Memory pools from #SSM_MEM_POOL_MIN to #SSM_MEM_POOL_MAX. */
struct mem_pool mem_pools[SSM_MEM_POOL_COUNT];

/** @brief Page allocation handler, set by ssm_mem_init(). */
static void *(*alloc_page)(void);

/** @brief Large memory allocation handler, set by ssm_mem_init(). */
static void *(*alloc_mem)(size_t size);

/** @brief Large memory release handler, set by ssm_mem_init(). */
static void (*free_mem)(void *mem, size_t size);

/** @brief Find the memory pool for some arbitrary size.
 *
 *  Tries to find the smallest memory pool that will fit a block of @a size.
 *  Returns #SSM_MEM_POOL_COUNT if no such pool exists, i.e., if @a size is
 *  greater than #SSM_MEM_POOL_MAX.
 *
 *  Since #SSM_MEM_POOL_COUNT is a (small) compile-time constant, this linear
 *  search is effectively contant time.
 *
 *  @param size   the block size whose memory pool we are looking for.
 *  @returns      index to the memory pool, or #SSM_MEM_POOL_COUNT otherwise.
 */
static inline size_t find_pool_size(size_t size) {
  for (size_t pool_idx = 0; pool_idx < SSM_MEM_POOL_COUNT; pool_idx++)
    if (size < SSM_MEM_POOL_SIZE(pool_idx))
      return pool_idx;
  return SSM_MEM_POOL_COUNT;
}

/** @brief Allocate a new block for a memory pool.
 *
 *  Calls alloc_page() to allocate a new zero-initialized page for the memory
 *  pool, and adds it to the corresponding memory pool with stack discipline.
 *  The last block of the free list is pointed to the old head of the free list.
 *
 *  Constant time (not counting what alloc_page() does).
 *
 *  @param p  the index of the memory pool.
 */
static inline void alloc_pool(size_t p) {
  block_t *new_page = alloc_page();
  SSM_ASSERT(END_OF_FREELIST < new_page);

  struct mem_pool *pool = &mem_pools[p];
  size_t last_block = BLOCKS_PER_PAGE - SSM_MEM_POOL_SIZE(p) / sizeof(block_t);
  new_page[last_block].free_list_next = pool->free_list_head;
  pool->free_list_head = new_page;
}

void ssm_mem_init(void *(*alloc_page_handler)(void),
                  void *(*alloc_mem_handler)(size_t),
                  void (*free_mem_handler)(void *, size_t)) {
  alloc_page = alloc_page_handler;
  alloc_mem = alloc_mem_handler;
  free_mem = free_mem_handler;

  for (size_t p = 0; p < SSM_MEM_POOL_COUNT; p++)
    mem_pools[p].free_list_head = END_OF_FREELIST;
}

void ssm_mem_prealloc(size_t size, size_t num_pages) {
  size_t p = find_pool_size(size);
  if (p >= SSM_MEM_POOL_COUNT)
    return;
  for (size_t i = 0; i < num_pages; i++)
    alloc_pool(p);
}

void *ssm_mem_alloc(size_t size) {
#ifdef SSM_DEBUG_NO_ALLOC
  return alloc_mem(size);
#else
  size_t p = find_pool_size(size);
  if (p >= SSM_MEM_POOL_COUNT)
    return alloc_mem(size);

  struct mem_pool *pool = &mem_pools[p];

  if (pool->free_list_head == END_OF_FREELIST)
    alloc_pool(p);

  void *buf = pool->free_list_head->block_buf;

  if (pool->free_list_head->free_list_next == UNINITIALIZED_FREE_BLOCK)
    pool->free_list_head += SSM_MEM_POOL_SIZE(p) / sizeof(block_t);
  else
    pool->free_list_head = pool->free_list_head->free_list_next;

  return buf;
#endif
}

void ssm_mem_free(void *m, size_t size) {
#ifdef SSM_DEBUG_NO_ALLOC
  free_mem(m, size);
#else
  size_t pool = find_pool_size(size);
  if (pool >= SSM_MEM_POOL_COUNT) {
    free_mem(m, size);
    return;
  }

  block_t *new_head = m;
  new_head->free_list_next = mem_pools[pool].free_list_head;
  mem_pools[pool].free_list_head = new_head;
#endif
}

/** @brief Recursively drop all children of a heap object.
 *
 *  @note assumes @a v is a valid @a heap_ptr.
 */
static inline void drop_children(ssm_value_t v) {
  if (!ssm_is_builtin(v.heap_ptr->tag)) {
    for (size_t i = 0; i < v.heap_ptr->val_count; i++)
      ssm_drop(ssm_adt_val(v, i));
  } else {
    switch (v.heap_ptr->tag) {
    case SSM_SV_T:
      ssm_unschedule(ssm_to_sv(v));
      ssm_drop(ssm_to_sv(v)->value);
      if (ssm_to_sv(v)->later_time != SSM_NEVER)
        ssm_drop(ssm_to_sv(v)->later_value);
      break;
    }
  }
}

ssm_value_t ssm_new_unsafe(uint8_t tag, uint8_t val_count, uint8_t val_offset) {
  struct ssm_mm *mm = ssm_mem_alloc(SSM_OBJ_SIZE(val_count, val_offset));
  mm->ref_count = 1;
  mm->tag = tag;
  mm->val_count = val_count;
  mm->val_offset = val_offset;
  return (ssm_value_t){.heap_ptr = mm};
}

void ssm_dup_unsafe(ssm_value_t v) { ++v.heap_ptr->ref_count; }

void ssm_drop_unsafe(ssm_value_t v) {
  if (--v.heap_ptr->ref_count == 0) {
    drop_children(v);
    ssm_mem_free(v.heap_ptr, ssm_obj_size(v));
  }
}

ssm_value_t ssm_reuse_unsafe(ssm_value_t v, uint8_t tag, uint8_t val_count,
                             uint8_t val_offset) {
  if (--v.heap_ptr->ref_count == 0) {
    drop_children(v);
    v.heap_ptr->ref_count = 1;
    v.heap_ptr->tag = tag;
    v.heap_ptr->val_count = val_count;
    v.heap_ptr->val_offset = val_offset;
    return v;
  } else {
    return ssm_new_unsafe(tag, val_count, val_offset);
  }
}
