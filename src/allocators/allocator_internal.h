#pragma once
#include "../../include/cutl/common_defs.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/*
 * Shared plumbing for the allocators in this library.
 *
 * The allocators which carve blocks out of a larger region all lay their blocks out the same way: a
 * block is a stretch of memory large enough to serve what the caller asked for, plus whatever the
 * allocator needs to keep track of the block, plus optional guard bytes around the memory the
 * caller gets to see. The OS allocator is the one exception, as it hands out whole pages rather than
 * cutting blocks out of them.
 *
 *   block start                                                                  block end
 *   |------------------------------- guards ------------------------------------|
 *   | [ allocator bookkeeping ] [ front guard | usable memory | rear guard ] [ ... ] |
 *   |                                 ^                                        ^
 *   |                                 the pointer handed to the caller           |
 *
 * Guard bytes and the alignment are compile-time choices, so that a build that does not need them
 * pays nothing for them at all. See the top level CMakeLists for how they are set.
 */

// Alignment a single allocation is guaranteed to have. A project may make this stricter, but never
// looser, as the allocators round all of their sizes by it.
#ifndef CUTL_ALLOCATOR_ALIGNMENT
#define CUTL_ALLOCATOR_ALIGNMENT alignof(max_align_t)
#endif

// Guard bytes placed in front of, and behind, every allocation. Zero disables them entirely.
#ifndef CUTL_ALLOCATOR_GUARD_BYTES
#define CUTL_ALLOCATOR_GUARD_BYTES 0
#endif

// The preprocessor needs to know whether guards do anything at all, and `#if` cannot see enums.
#if CUTL_ALLOCATOR_GUARD_BYTES > 0
#define CUTL_ALLOCATOR_GUARDS_ENABLED 1
#else
#define CUTL_ALLOCATOR_GUARDS_ENABLED 0
#endif

enum
{
    // Alignment every allocation is rounded to, and the alignment it is handed out with.
    ALLOCATOR_ALIGNMENT = CUTL_ALLOCATOR_ALIGNMENT,
    ALLOCATOR_ALIGNMENT_MASK = ALLOCATOR_ALIGNMENT - 1,

    // Guards are rounded up to the alignment, so that the memory handed to the caller stays aligned
    // no matter how many guard bytes were asked for.
    ALLOCATOR_GUARD_BYTES = (CUTL_ALLOCATOR_GUARD_BYTES + ALLOCATOR_ALIGNMENT_MASK) & ~ALLOCATOR_ALIGNMENT_MASK,
    // Guards sit on both ends of a block.
    ALLOCATOR_GUARD_TOTAL = 2 * ALLOCATOR_GUARD_BYTES,
    // Value the guard bytes are filled with. A pattern that is not a valid address, so that walking
    // off the end of a block is likely to fault rather than quietly read something.
    ALLOCATOR_GUARD_PATTERN = 0xCC,
};

// Constant expression form of `allocator_round_up`. C has no constant functions, so this is the only
// way to round something that is already known at compile time.
#define CUTL_ALLOCATOR_ROUND_UP(size) (((size) + ALLOCATOR_ALIGNMENT_MASK) & ~(size_t)ALLOCATOR_ALIGNMENT_MASK)

typedef enum : uint8_t
{
    MEMORY_BLOCK_FREE,   // Block can be used to allocate memory
    MEMORY_BLOCK_USED,   // Block is currently used for an allocation
    MEMORY_BLOCK_MERGED, // Block is in the process of being merged
} memory_block_state_t;

/**
 * Check whether an address is aligned the way the allocators hand out memory.
 *
 * @param mem Address to check.
 * @return Non-zero if the address is aligned, zero if it is not.
 */
static inline int allocator_is_aligned(const void *const mem)
{
    return ((uintptr_t)mem & ALLOCATOR_ALIGNMENT_MASK) == 0;
}

/**
 * Round a size up to the next multiple of the allocation alignment.
 *
 * @param size Size to round up.
 * @return Size rounded up to the allocation alignment.
 */
static inline size_t allocator_round_up(const size_t size)
{
    return CUTL_ALLOCATOR_ROUND_UP(size);
}

/**
 * Round a size down to the previous multiple of the allocation alignment.
 *
 * @param size Size to round down.
 * @return Size rounded down to the allocation alignment.
 */
static inline size_t allocator_round_down(const size_t size)
{
    return size & ~(size_t)ALLOCATOR_ALIGNMENT_MASK;
}

/**
 * Get the total size a block needs in order to serve the given amount of usable memory, which
 * includes the guard bytes on both of its ends.
 *
 * @param usable Amount of memory the caller is going to use.
 * @return Total size of the block, in bytes.
 */
static inline size_t allocator_block_size(const size_t usable)
{
    return allocator_round_up(usable) + ALLOCATOR_GUARD_TOTAL;
}

/**
 * Get the amount of memory a caller can actually use in a block of the given total size.
 *
 * @param total Total size of the block, in bytes.
 * @return Amount of memory within the block that is not taken up by guards.
 */
static inline size_t allocator_usable_size(const size_t total)
{
    return total - ALLOCATOR_GUARD_TOTAL;
}

/**
 * Get the offset from the start of a block to the memory handed to the caller. This is the size of
 * the front guard, and is zero for blocks without guards.
 *
 * @return Offset of the usable memory from the start of the block.
 */
static inline size_t allocator_user_offset(void)
{
    return ALLOCATOR_GUARD_BYTES;
}

/**
 * Get the start of the block a piece of memory handed to a caller belongs to.
 *
 * @param memory Memory handed out by an allocator.
 * @return Address the block starts at.
 */
static inline void *allocator_block_start(void *const memory)
{
    return (unsigned char *)memory - ALLOCATOR_GUARD_BYTES;
}

/**
 * Fill the guard bytes on both ends of a block, marking the block as being in use.
 *
 * Does nothing at all if the build was made without guard bytes.
 *
 * @param block Address the block starts at.
 * @param total Total size of the block, in bytes.
 */
static inline void allocator_set_guards(void *const block, const size_t total)
{
#if CUTL_ALLOCATOR_GUARDS_ENABLED
    unsigned char *const bytes = (unsigned char *)block;
    memset(bytes, ALLOCATOR_GUARD_PATTERN, ALLOCATOR_GUARD_BYTES);
    memset(bytes + total - ALLOCATOR_GUARD_BYTES, ALLOCATOR_GUARD_PATTERN, ALLOCATOR_GUARD_BYTES);
#else
    (void)block;
    (void)total;
#endif
}

/**
 * Check that the guard bytes on both ends of a block are still intact, which tells us whether
 * anything wrote outside of the memory the caller was given.
 *
 * Always reports the block as intact if the build was made without guard bytes.
 *
 * @param block Address the block starts at.
 * @param total Total size of the block, in bytes.
 * @return Non-zero if the guards are intact, zero if anything overwrote them.
 */
static inline int allocator_check_guards(const void *const block, const size_t total)
{
#if CUTL_ALLOCATOR_GUARDS_ENABLED
    const unsigned char *const bytes = (const unsigned char *)block;
    for (size_t i = 0; i < ALLOCATOR_GUARD_BYTES; ++i)
    {
        // The guards on either end are written in the same order, so they can be checked together.
        if (bytes[i] == ALLOCATOR_GUARD_PATTERN && bytes[total - 1 - i] == ALLOCATOR_GUARD_PATTERN)
            continue;

        const char *const overwritten = bytes[i] != ALLOCATOR_GUARD_PATTERN ? "front" : "rear";
        fprintf(stderr, "Memory block %p had its %s guard overwritten (expected %02X, found",
                (void *)((const uintptr_t)block + ALLOCATOR_GUARD_BYTES), overwritten,
                (unsigned char)ALLOCATOR_GUARD_PATTERN);
        for (size_t j = 0; j < ALLOCATOR_GUARD_BYTES; ++j)
        {
            fprintf(stderr, " %02X", (unsigned char)bytes[i < ALLOCATOR_GUARD_BYTES / 2 ? j : total - 1 - j]);
        }
        fprintf(stderr, ").\n");
        return 0;
    }
    return 1;
#else
    (void)block;
    (void)total;
    return 1;
#endif
}
