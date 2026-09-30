Allocators
==========

Allocators are types and associated functions that help deal with dynamic
memory management. While there are a few different types, with each intended for
different use case, the most basic allocators rely on being given an allocated chunk of memory
to manage. As such they do not really "own" the memory they deal with.

Every allocation is aligned to ``max_align_t``, and most of the allocators additionally place
guard bytes around the memory they hand out, which is how a write outside an allocation gets
reported instead of quietly damaging whatever sits next in memory. Both are compile-time choices,
covered in :doc:`allocators/build_options`.

.. toctree::
    :caption: Allocators
    :maxdepth: 1

    allocators/interface
    allocators/block_allocator
    allocators/fixed_size_allocator
    allocators/arena_allocator
    allocators/build_options
    allocators/stack_allocator
