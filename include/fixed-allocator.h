#ifndef _FIXED_ALLOCATOR_H
#define _FIXED_ALLOCATOR_H

#include <assert.h>
#include <fixed-allocator.h>
#include <ssm.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

typedef void *memory_t;

typedef struct fixed_allocator {
  size_t blockSize; // in bytes
  size_t numBlocks;
  memory_t memoryPool;
  memory_t freeListHead;
} fixed_allocator_t;
/**
  The Fixed Allocator:

  @brief The fixed allocator performs equivalent function to malloc and free,
  however it only allocates blocks of a given size.

  It internally maintains a freelist,  which is essentially a linked list,
  where each free block points to the next
  block that is free, with a pointer in the first word of each block.

  Due to the free list implementation, we require that the size is bigger than
  the size of a pointer.

*/
/**
  @brief fa_initialize sets up the allocator to have a certain size block, and
  a certain number of blocks.

  The free list is set up, and a pointer to the allocator is returned.

  @param blockSize - number of chars the allocator returns on an an allocation
  @param numBlocks - number of blocks this allocator holds
  @param memory - raw memory on which to build the allocator, must be
  at least numBlocks*blockSize+sizeof(fixed_allocator_t) big.
*/
fixed_allocator_t *fa_initialize(size_t blockSize, size_t numBlocks,
                                 void *memory);

/**
  @brief fa_malloc returns the next block from the allocator's freelist.

  Returns NULL if OOM (this is protected by an assertion).
  Free list is adjusted accordingly.

  @param allocator - the allocator from which to return memory
*/
memory_t fa_malloc(fixed_allocator_t *allocator);

/**

  @brief fa_free returns block of memory to the allocator.

  Free list is adjusted accordingly.
  The behaviour is undefined if the block is outside the bounds of the
  allocator, or not aligned to a block the allocator knows about.

  @param allocator - allocator to which to return the memory
  @param address - pointer to the start of the memory region to be freed
*/
void fa_free(fixed_allocator_t *allocator, memory_t address);

/*
  @brief fa_destroy is the destructor for an allocator
  Currently a no-op.
*/
void fa_destroy(fixed_allocator_t *allocator);

#endif
