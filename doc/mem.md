See @ref mem.

@defgroup mem Memory management

The SSM runtime uses reference counted memory management, based on the [Perceus algorithm](https://www.microsoft.com/en-us/research/publication/perceus-garbage-free-reference-counting-with-reuse/). It exposes memory management primitives ssm_new(), ssm_dup(), ssm_drop(), and ssm_reuse().

For simplicity and flexibility, all values in SSM runtime are word-sized, represented using #ssm_value_t. When the size of a value exceeds what can be represented by a word, the value is a pointer to a heap-allocated object. In particular, they point to a memory management metadata header, #ssm_mm, which contains enough information to surmise the reference count and extant of that object. The object may be a basic tuple of other #ssm_value_t values, indicated by the @a val_count field, or it may be a builtin, when the @a val_count field is #SSM_BUILTIN.

The SSM runtime comes with its own platform-agnostic allocator, parameterized by handlers set using ssm_mem_init(). It allocates small pieces of memory in <em>blocks</em>, within designated <em>memory pools</em>. Each memory pool consists of zero or more fixed-size <em>memory pages</em>, and are requested from the platform/OS on-demand via the alloc_page() handler. Meanwhile, allocations for larger ranges of memory are deferred to the alloc_mem() and free_mem() handlers.

The allocator's memory pools may be configured using the #SSM_MEM_POOL_MIN, #SSM_MEM_POOL_FACTOR_BASE2, #SSM_MEM_POOL_COUNT, and #SSM_MEM_PAGE_SIZE preprocessor constants. The default values produce 4 memory pools, of sizes 16B, 64B, 256B, and 1024B, and a memory page size of 4096B.
