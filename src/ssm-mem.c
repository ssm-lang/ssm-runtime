/** @file ssm-mem.c
 *  @brief SSM runtime memory management.
 *
 *  @author John Hui (j-hui)
 */
#include <allocation-dispatcher.h>
#include <assert.h>
#include <fixed-allocator.h>
#include <ssm.h>
#include <ssm-internal.h>
#include <stdio.h>
#include <stdlib.h>

allocation_dispatcher_t *ad = NULL;

void ssm_mem_init(size_t allocator_sizes[], size_t allocator_blocks[],
                  size_t num_allocators) {
  int size_to_malloc = 0;
  for (int i = 0; i < num_allocators; i++) {
    size_to_malloc +=
        allocator_sizes[i] * allocator_blocks[i] * sizeof(ssm_word_t);
  }
  void *memory_block = malloc(size_to_malloc);
  allocation_dispatcher_t *dispatcher = ad_initialize(
      allocator_sizes, allocator_blocks, num_allocators, memory_block);
  ad = dispatcher;
}

struct ssm_mm *ssm_mem_alloc(size_t size) {
  return ad_malloc(ad, size); // toDONE(tm): (dan) use our own allocator
}

void ssm_mem_free(struct ssm_mm *mm, size_t size) {
  ad_free(ad, size, mm); // toDONE(tm): (dan) use our own allocator
}

struct ssm_mm *ssm_new_builtin(enum ssm_builtin builtin) {
  struct ssm_mm *mm = ssm_mem_alloc(SSM_BUILTIN_SIZE(builtin));
  mm->val_count = SSM_BUILTIN;
  mm->tag = builtin;
  mm->ref_count = 1;
  return mm;
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
  if (--mm->ref_count == 0)
    ssm_mem_free(mm, ssm_mm_is_builtin(mm) ? SSM_BUILTIN_SIZE(mm->tag)
                                           : SSM_OBJ_SIZE(mm->val_count));
}

struct ssm_mm *ssm_reuse(struct ssm_mm *mm) {
  SSM_ASSERT(0); // TODO: not implemented
  return NULL;
}

fixed_allocator_t *fa_initialize(size_t blockSize, size_t numBlocks,
                                 void *memory) {
  assert(blockSize > sizeof(ssm_word_t));
  // need at least 1 word for pointer to next block in freelist
  fixed_allocator_t *allocator = memory;
  void *pool = ((char *)memory) + sizeof(fixed_allocator_t);
  allocator->blockSize = blockSize;
  allocator->numBlocks = numBlocks;
  allocator->memoryPool = pool;
  allocator->freeListHead = allocator->memoryPool;

  // initalize freelist
  ssm_word_t currentAddress = (ssm_word_t)(allocator->memoryPool);
  for (int i = 0; i < numBlocks - 1; i++) {
    *((ssm_word_t *)currentAddress) = currentAddress + blockSize;
    currentAddress += blockSize;
  }
  *((ssm_word_t *)currentAddress) = 0; // nullptr - no more free
  return allocator;
}
memory_t fa_malloc(fixed_allocator_t *allocator) {
  assert(allocator->freeListHead != 0); // fail on oom
  memory_t toReturn = allocator->freeListHead;
  allocator->freeListHead =
      (memory_t)(*((ssm_word_t *)(allocator->freeListHead)));
  return toReturn;
}
void fa_free(fixed_allocator_t *allocator, memory_t address) {
  *((ssm_word_t *)address) = (ssm_word_t)(allocator->freeListHead);
  allocator->freeListHead = address;
}

void fa_destroy(fixed_allocator_t *allocator) {}
