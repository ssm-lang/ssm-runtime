#ifndef ALLOCATIONDISPATCHER
#define ALLOCATIONDISPATCHER
#include "fixedAllocator.h"
#include "constants.h"
#include <stddef.h>
#include <stdio.h>

typedef struct AllocatorDispatcher {
  int numAllocators;
  FIXED_ALLOCATOR **allocators;
} ALLOCATION_DISPATCHER;

//if malloc is not allowed, this will throw an assertion
#define ALLOW_MALLOC 0

/*
  Allocation Dispatcher:

  This is the top-level malloc replacement system.
  The allocation dispatcher allocates space in one of its child allocators.
  The child allocators make fixed-size allocations, and the dispatcher
  will select the apropriate child allocator.
*/

/*
  adInitialize:

  adInitialize sets up the dispatcher, initializing underlying fixed allocators
*/
ALLOCATION_DISPATCHER* adInitialize(size_t blockSizes[], size_t numBlocks[], size_t numAllocators, void* memoryPool);

/*
  adMalloc:

  adMalloc is the malloc replacement - it will allocate the requested size
  in one of the child allocators, returning a buffer (void*) with at least as
  much available space as requested.

  Size is in number of bytes (sizeof(char)), much like normal malloc
*/
void* adMalloc(ALLOCATION_DISPATCHER *allocationDispatcher, size_t size);

/*
  adFree:

  adFree is the equivalent of free - it returns the memory passed to the
  allocator which is responsible for blocks of the given size.

  At the moment, it uses the passed size argument to determine the allocator,
  however, the address itself could also be used.

*/
void* adFree(ALLOCATION_DISPATCHER *allocationDispatcher, size_t size, void* memory);
/*
 adDestroy:

 destroys underlying allocators before also freeing itself.
 This may go away if a block of memory is given to the dispatcher to distribute
 to the fixed allocators.
*/
void adDestroy(ALLOCATION_DISPATCHER *dispatcher);

/*
  findAllocator:

  internal helper function to figure out the underlying allcator to make allocation.
  this may go away eventually
*/
FIXED_ALLOCATOR* findAllocator(ALLOCATION_DISPATCHER *allocationDispatcher, size_t size);
#endif
