#ifndef _ALLOCATION_DISPATCHER_H
#define _ALLOCATION_DISPATCHER_H
#include <fixed-allocator.h>
#include <ssm.h>
#include <stddef.h>
#include <stdio.h>

typedef struct allocation_dispatcher {
  int num_allocators;
  fixed_allocator_t **allocators;
} allocation_dispatcher_t;

/**
  @brief allocation_dispatcher is the top-level malloc replacement system.
  The allocation dispatcher allocates space in one of its child allocators.
  The child allocators make fixed-size allocations, and the dispatcher
  will select the apropriate child allocator.
*/

/**
  @brief ad_initialize sets up the dispatcher, initializing underlying fixed
  allocators

  @param blockSizes - number of chars each child allocator returns on an an
  allocation
  @param numBlocks - number of blocks each child allocator holds
  @param memoryPool - raw memory on which to build the allocators, must be
  at least equal to the sum of numBlocks*blockSize+sizeof(fixed_allocator_t)
  for each allocator plus sizeof the allocation_dispatcher itself.
*/
allocation_dispatcher_t *ad_initialize(size_t block_sizes[],
                                       size_t num_blocks[],
                                       size_t num_allocators,
                                       void *memory_pool);

/**
  @brief ad_malloc is the malloc replacement - it will allocate the requested
  size in one of the child allocators, returning a buffer (void*) with at least
  as much available space as requested.

  Size is in number of bytes (sizeof(char)), much like normal malloc.
  Assertion error if a suitable allocator is not found.

  @param allocationDispatcher - this is the dispatcher to use to get memory
  @param size - the size in bytes of the allocation (same as malloc)
*/
void *ad_malloc(allocation_dispatcher_t *allocation_dispatcher, size_t size);

/**
  @brief ad_free is the equivalent of free - it returns the memory passed to the
  allocator which is responsible for blocks of the given size.

  At the moment, it uses the passed size argument to determine the allocator,
  however, the address itself could also be used.

  @param allocationDispatcher - this is the dispatcher which was used
  to get the memory
  @param size - the size in bytes of the allocation
  (must be the same as was passed to ad_malloc, used to find the right
  allocator)
  @param memory - this is the block to be freed

*/
void ad_free(allocation_dispatcher_t *allocation_dispatcher, size_t size,
              void *memory);
/**
  @brief the destructor for the allocation_dispatcher

  Currently a no-op.

  @param dispatcher - the dispatcher to free
*/
void ad_destroy(allocation_dispatcher_t *dispatcher);

/**

  @brief find_allocator is an internal helper function to figure out the
  underlying allcator used to make the  allocation.

  @param allocationDispatcher - the dispatcher to search for an allocator
  @param size - the size of the allocation the allocator makes
*/
fixed_allocator_t *
find_allocator(allocation_dispatcher_t *allocation_dispatcher, size_t size);
#endif
