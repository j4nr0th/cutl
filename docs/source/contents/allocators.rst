Allocators
==========

Allocators are types and associated functions that help deal with dynamic
memory management. While there are a few different types, with each intended for
different use case, the most basic allocators rely on being given an allocated chunk of memory
to manage. As such they do not really "own" the memory they deal with.

.. toctree::
    :caption: Allocators
    :maxdepth: 1

    allocators/interface
    allocators/block_allocator
    allocators/fixed_size_allocator
    allocators/stack_allocator
