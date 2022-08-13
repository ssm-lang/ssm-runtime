See @ref blob.

@defgroup blob Blobs

The SSM allocator allows programs to allocate contiguous chunks of
reference-counted memory whose contents are of arbitrary layout.
This object type provides users the flexibility to extend runtime with objects
that the runtime does not natively support, so long as they commit to managing
stored resources themselves, because the garbage collector will not scan the
payload for other managed heap pointers.

Since there is no meaningful interpretation to the @a tag field of the memory
management header, blobs use the @a size field of its #ssm_mm header.
