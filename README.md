# CUTL Library

This is a C library containing utilities which I have found myself often wanting in other C projects I made. While the library itself uses C23 features to build, its headers should be possible to use even on C99 projects. There is no real goal for what are all features that should be in this library, so I will just add them as I run into more of those I think should be in it.

## Current Features

The features of the library can currently be split into three main categories:

- Allocators
- Formatted Output
- Iteration

### Allocators

Just about all allocators in this library are basic building blocks that can
either be used on their own, or used as basis of more complex allocators. None are thread-safe and just about all need to be given an already allocated chunk of memory to manage. As such, the intended use for them is to allocate memory from the OS, then give it to them to manage.

The arena allocator is the exception, as it takes its memory from another allocator instead of from
the caller, in large chunks that it then carves up itself. That makes it well suited for scratch
buffers which are allocated in bulk and released in one go once the function that needed them is
done, either keeping the memory around for reuse or handing all of it back at once.

Every allocation is aligned to `max_align_t`, and most of the allocators additionally place guard
bytes around the memory they hand out, which is what turns a write outside of an allocation into a
report instead of silent damage to whatever sits next in memory. Both, along with the internal
consistency checks, are compile-time choices set through CMake, and default to off in a `Release`
build. A project pulling this one in with `add_subdirectory()` stays in charge of them, as none of
the options are given a default if it has already set one itself.


### Formatted Output

While I do not mind the standard library's IO, I did find it a bit annoying to specify how to format numbers very precisely. I was also not the greatest fan of how it used null-terminated strings. What I was the most upset over was the whole standard library's locale concept. All formatting functions depend on an opaque global (or for some versions thread-local) setting. As such, all formatting functions can be told in great detail every detail of how to format numbers. This is of course likely comes at cost of performance, but if formatting is such a performance bottleneck for you, you might as well copy these formatting functions and hardcode the characters you intend to use.

### Iteration

When dealing with differential geometry I always ended up having to iterate over combinations and permutations of covector basis in bundles. As such, I figured that having iterators for both was essential. The permutation iterator has the nice bonus of being able to tell you if an even or an odd number of swaps is needed to order all elements, while the combination iterator can tell you what was the index of any combination you have.
