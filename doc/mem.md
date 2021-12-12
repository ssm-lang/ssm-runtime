See @ref mem.

@defgroup mem Memory management


The SSM runtime comes with its own platform-agnostic allocator, parameterized by handlers set using ssm_mem_init(). It allocates small pieces of memory in <em>blocks</em>, within designated <em>memory pools</em>. Each memory pool consists of zero or more fixed-size <em>memory pages</em>, and are requested from the platform/OS on-demand via the alloc_page() handler. Meanwhile, allocations for larger ranges of memory are deferred to the alloc_mem() and free_mem() handlers.

The allocator's memory pools may be configured using the #SSM_MEM_POOL_MIN, #SSM_MEM_POOL_FACTOR_BASE2, #SSM_MEM_POOL_COUNT, and #SSM_MEM_PAGE_SIZE preprocessor constants. The default values produce 4 memory pools, of sizes 16B, 64B, 256B, and 1024B, and a memory page size of 4096B.
