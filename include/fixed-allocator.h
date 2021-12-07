#ifndef _FIXED_ALLOCATOR_H
#define _FIXED_ALLOCATOR_H

#include <stddef.h>
#include <stdint.h>

// initialize func allocates space to a global var
// skip list initialized with first word pointing to next free
// global next free pointer - head
// when freeing, insert at head of linkedlist (reuse!, reduce!, recycle!)
// actually - not a global var - i guess it's a struct called the fixed_allocator
// this struct holds memory, and a fixed size

typedef void* MEMORY;


#if UINTPTR_MAX == 0xFFFF
  #error how do you even metadata?
#elif INTPTR_MAX == INT32_MAX
  typedef uint32_t INTEGER_REPRESENTATION;
#elif INTPTR_MAX == INT64_MAX
  typedef unsigned long INTEGER_REPRESENTATION;
#else
  #error Unknown pointer size
#endif

typedef struct fixed_allocator {
  size_t blockSize; // in bytes
  size_t numBlocks;
  MEMORY memoryPool;
  MEMORY freeListHead;
} fixed_allocator_t;
/*
  The Fixed Allocator:

  The fixed allocator performs equivalent function to malloc and free, however
  it only allocates blocks of a given size. It internally maintains a freelist
  which is essentially a linked list, where each free block points to the next
  block that is free, with a pointer in the first word of each block.

  Due to the free list implementation, we require that the size is bigger than the
  size of a pointer.

*/
/*
  faInitialize:

  Initialize sets up the allocator to have a certain size block, and
  a certain number of blocks. The free list is set up, and a pointer to the
  allocator is returned.
*/
fixed_allocator_t *faInitialize(size_t blockSize, size_t numBlocks, void* memory);

/*
  faMalloc:

  Returns the next block from the freelist.
  Returns NULL if OOM (this is protected by an assertion).
  Free list is adjusted accordingly.
*/
MEMORY faMalloc(fixed_allocator_t *allocator);

/*
  faFree:

  Returns block of memory to the allocator.
  Free list is adjusted accordingly.
  The behaviour is undefined if the block is outside the bounds of the allocator,
  or not aligned to a block teh allocator knows about.
*/
void faFree(fixed_allocator_t *allocator, MEMORY address);

/*
  faDestroy:

  Returns the memory fo the allocator to the system. This may be deprecated
  if the allocator is passed a block of memory to manage.
*/
void faDestroy(fixed_allocator_t *allocator);

#endif
