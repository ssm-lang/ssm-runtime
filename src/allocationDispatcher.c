#include "allocationDispatcher.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

ALLOCATION_DISPATCHER* adInitialize(size_t blockSizes[], size_t numBlocks[], size_t numAllocators, void* memoryPool){
  void* memoryHead = memoryPool;
  ALLOCATION_DISPATCHER* dispatcher= memoryHead;
  memoryHead = ((char*) memoryHead)+sizeof(ALLOCATION_DISPATCHER);
  dispatcher->numAllocators = numAllocators;
  dispatcher->allocators = memoryHead;
  memoryHead= ((char*) memoryHead)+sizeof(FIXED_ALLOCATOR)*numAllocators;
  for(int i=0; i<numAllocators; i++){
    void* memory = memoryHead;
    memoryHead = ((char*) memoryHead)+blockSizes[i]*numBlocks[i]+sizeof(FIXED_ALLOCATOR);
    dispatcher->allocators[i]=faInitialize(blockSizes[i], numBlocks[i], memory);
  }
  return dispatcher;
}

void adDestroy(ALLOCATION_DISPATCHER* ad){
  for(int i=0; i<ad->numAllocators; i++){
    faDestroy(ad->allocators[i]);
  }
}
void* adMalloc(ALLOCATION_DISPATCHER *ad, size_t size){
  FIXED_ALLOCATOR *allocator = findAllocator(ad, size);
  if(!allocator){
    printf("\n\nmissing size %d\n\n",size);
    assert(0==size);
  }
  return faMalloc(allocator);
}
void* adFree(ALLOCATION_DISPATCHER *ad, size_t size, void* memory){
  FIXED_ALLOCATOR *allocator = findAllocator(ad, size);
  if(!allocator){
    #if !ALLOW_MALLOC
      assert(0);
    #endif
    free(memory);
  }
  else{
    faFree(allocator,memory);
  }
}
FIXED_ALLOCATOR* findAllocator(ALLOCATION_DISPATCHER *ad, size_t size){
  //todo: sort and binary search?
  //todo: smallest available size above threshold?
  for(int i=0; i<ad->numAllocators; i++){
    if(ad->allocators[i]->blockSize==size){
      return ad->allocators[i];
    }
  }
  return NULL;
}
