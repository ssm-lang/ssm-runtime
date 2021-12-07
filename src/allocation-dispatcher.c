#include <allocation-dispatcher.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

allocation_dispatcher_t* adInitialize(size_t blockSizes[], size_t numBlocks[], size_t numAllocators, void* memoryPool){
  void* memoryHead = memoryPool;
  allocation_dispatcher_t* dispatcher= memoryHead;
  memoryHead = ((char*) memoryHead)+sizeof(allocation_dispatcher_t);
  dispatcher->numAllocators = numAllocators;
  dispatcher->allocators = memoryHead;
  memoryHead= ((char*) memoryHead)+sizeof(fixed_allocator_t)*numAllocators;
  for(int i=0; i<numAllocators; i++){
    void* memory = memoryHead;
    memoryHead = ((char*) memoryHead)+blockSizes[i]*numBlocks[i]+sizeof(fixed_allocator_t);
    dispatcher->allocators[i]=faInitialize(blockSizes[i], numBlocks[i], memory);
  }
  return dispatcher;
}

void adDestroy(allocation_dispatcher_t* ad){
  for(int i=0; i<ad->numAllocators; i++){
    faDestroy(ad->allocators[i]);
  }
}
void* adMalloc(allocation_dispatcher_t *ad, size_t size){
  fixed_allocator_t *allocator = findAllocator(ad, size);
  if(!allocator){
    printf("\n\nmissing size %d\n\n",size);
    assert(0==size);
  }
  return faMalloc(allocator);
}
void* adFree(allocation_dispatcher_t *ad, size_t size, void* memory){
  fixed_allocator_t *allocator = findAllocator(ad, size);
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
fixed_allocator_t* findAllocator(allocation_dispatcher_t *ad, size_t size){
  //todo: sort and binary search?
  //todo: smallest available size above threshold?
  for(int i=0; i<ad->numAllocators; i++){
    if(ad->allocators[i]->blockSize==size){
      return ad->allocators[i];
    }
  }
  return NULL;
}
