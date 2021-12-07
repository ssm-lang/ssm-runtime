#include <fixed-allocator.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <ssm.h>
FIXED_ALLOCATOR *faInitialize(size_t blockSize, size_t numBlocks, void* memory){
  assert(blockSize>sizeof(ssm_word_t));
    //need at least 1 word for pointer to next block in freelist
  FIXED_ALLOCATOR* allocator = memory;
  void* pool = ((char*) memory)+sizeof(FIXED_ALLOCATOR);
  allocator->blockSize = blockSize;
  allocator->numBlocks = numBlocks;
  allocator->memoryPool = pool;
  allocator->freeListHead = allocator->memoryPool;

  // initalize freelist
  INTEGER_REPRESENTATION currentAddress = (INTEGER_REPRESENTATION)(allocator->memoryPool);
  //fprintf(DEBUG_DEST,"allocator %ld\n",(INTEGER_REPRESENTATION)allocator);
  //fprintf(DEBUG_DEST,"Initializing free list\n(address, value)\n");
  for(int i=0; i<numBlocks-1; i++){
    *((INTEGER_REPRESENTATION*) currentAddress)=currentAddress+blockSize;
    //fprintf(DEBUG_DEST,"%ld, %ld\n",currentAddress, *((INTEGER_REPRESENTATION*) currentAddress));
    currentAddress+=blockSize;
  }
  *((INTEGER_REPRESENTATION*) currentAddress)=0; //nullptr - no more free
  //fprintf(DEBUG_DEST,"%ld, %ld\n",currentAddress, *((INTEGER_REPRESENTATION*) currentAddress));
  return allocator;
}
MEMORY faMalloc(FIXED_ALLOCATOR *allocator){
  assert(allocator->freeListHead!=0); //fail on oom
  MEMORY toReturn = allocator->freeListHead;
  allocator->freeListHead = (MEMORY) (*((INTEGER_REPRESENTATION*)(allocator->freeListHead)));
  return toReturn;
}
void faFree(FIXED_ALLOCATOR *allocator, MEMORY address){
  *((INTEGER_REPRESENTATION*) address) = (INTEGER_REPRESENTATION) (allocator->freeListHead);
  allocator->freeListHead = address;
}

void faDestroy(FIXED_ALLOCATOR *allocator){
}
