/** @file ssm-mem.c
 *  @brief SSM runtime memory management.
 *
 *  @author John Hui (j-hui)
 */
#include <ssm-internal.h>
#include <allocationDispatcher.h>
ALLOCATION_DISPATCHER *ad = NULL;
size_t allSizes[1000];
int numSizes = 0;
struct ssm_mm *ssm_mem_alloc(size_t size) {
  if(ad==NULL){
    size_t allocatorSizes[] = {48};
    size_t allocatorBlocks[] = {4000};
    size_t numAllocators = 1;
    void* memoryBlock =  malloc(48*1000*WORD_BOUNDARY);
    ALLOCATION_DISPATCHER *dispatcher = adInitialize(allocatorSizes,allocatorBlocks,numAllocators,memoryBlock);
    ad = dispatcher;
  }
  /*

  // this block of code is used to (inefficiently) construct an array of all allocated sizes

  int foundSize = 0;
  for(int i=0; i<numSizes;i++){
    if(size==allSizes[i]){
      foundSize=1;
    }
  }
  if(!foundSize){
    allSizes[numSizes++]=size;
  }
  printf("\nsizes:\n");
  for(int i=0; i<numSizes;i++){
    printf("%d, ",allSizes[i]);
  }
  printf("\n");
  */
  return adMalloc(ad,size); // toDONE(tm): (dan) use our own allocator
}

void ssm_mem_free(struct ssm_mm *mm, size_t size) {
  adFree(ad,size,mm); // toDONE(tm): (dan) use our own allocator
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
