#include <fixed-allocator.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <ssm.h>
fixed_allocator_t *fa_initialize(size_t blockSize, size_t numBlocks, void* memory){
  assert(blockSize>sizeof(ssm_word_t));
    //need at least 1 word for pointer to next block in freelist
  fixed_allocator_t* allocator = memory;
  void* pool = ((char*) memory)+sizeof(fixed_allocator_t);
  allocator->blockSize = blockSize;
  allocator->numBlocks = numBlocks;
  allocator->memoryPool = pool;
  allocator->freeListHead = allocator->memoryPool;

  // initalize freelist
  ssm_word_t currentAddress = (ssm_word_t)(allocator->memoryPool);
  for(int i=0; i<numBlocks-1; i++){
    *((ssm_word_t*) currentAddress)=currentAddress+blockSize;
    currentAddress+=blockSize;
  }
  *((ssm_word_t*) currentAddress)=0; //nullptr - no more free
  return allocator;
}
memory_t fa_malloc(fixed_allocator_t *allocator){
  assert(allocator->freeListHead!=0); //fail on oom
  memory_t toReturn = allocator->freeListHead;
  allocator->freeListHead = (memory_t) (*((ssm_word_t*)(allocator->freeListHead)));
  return toReturn;
}
void fa_free(fixed_allocator_t *allocator, memory_t address){
  *((ssm_word_t*) address) = (ssm_word_t) (allocator->freeListHead);
  allocator->freeListHead = address;
}

void fa_destroy(fixed_allocator_t *allocator){
}
