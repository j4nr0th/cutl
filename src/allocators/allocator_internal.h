#pragma once
#include "../common_defs.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef enum : unsigned char
{
    BYTE_GUARD_UNUSED = 0b01010101,
    BYTE_GUARD_FRESH = 0b10101010,
    BYTE_GUARD_IN_USE = 0b11001100,
} memory_block_guard_bytes_t;

typedef enum : unsigned char
{
    MEMORY_BLOCK_FREE,   // Block can be used to allocate memory
    MEMORY_BLOCK_USED,   // Block is currently used for an allocation
    MEMORY_BLOCK_MERGED, // Block is in the process of being merged
} memory_block_state_t;

enum
{
    ALLOCATOR_MINIMUM_ALIGNMENT = sizeof(void *),
    ALLOCATOR_MINIMUM_ALIGNMENT_MASK = ALLOCATOR_MINIMUM_ALIGNMENT - 1,
    ALLOCATOR_GUARD_BYTE_COUNT = ALLOCATOR_MINIMUM_ALIGNMENT,
};

static inline int _check_alignment(const void *const mem)
{
    const uintptr_t address = (uintptr_t)mem;
    return (address % ALLOCATOR_MINIMUM_ALIGNMENT) == 0;
}

static inline size_t _round_align_floor(const size_t size)
{
    return (size / ALLOCATOR_MINIMUM_ALIGNMENT) * ALLOCATOR_MINIMUM_ALIGNMENT;
}

static inline size_t _round_align_ceil(const size_t size)
{
    const size_t x = size % ALLOCATOR_MINIMUM_ALIGNMENT;
    const size_t y = size / ALLOCATOR_MINIMUM_ALIGNMENT;
    return (y + (x != 0)) * ALLOCATOR_MINIMUM_ALIGNMENT;
}

static inline size_t _get_block_size(const size_t size)
{
    return _round_align_ceil(_round_align_ceil(size) + 2LLU * ALLOCATOR_GUARD_BYTE_COUNT);
}

static inline void _prepare_block_fresh(const size_t size, void *const block)
{
    memset(block, BYTE_GUARD_UNUSED, ALLOCATOR_GUARD_BYTE_COUNT);
    memset(block + size - ALLOCATOR_GUARD_BYTE_COUNT, BYTE_GUARD_UNUSED, ALLOCATOR_GUARD_BYTE_COUNT);
}

static inline void _prepare_block_used(const size_t size, void *const block)
{
    memset(block, BYTE_GUARD_IN_USE, ALLOCATOR_GUARD_BYTE_COUNT);
    // memset(block + ALLOCATOR_GUARD_BYTE_COUNT, BYTE_GUARD_FRESH, size - 2LLU * ALLOCATOR_GUARD_BYTE_COUNT);
    memset(block + size - ALLOCATOR_GUARD_BYTE_COUNT, BYTE_GUARD_IN_USE, ALLOCATOR_GUARD_BYTE_COUNT);
}

static inline int _verify_block_in_use(const size_t size, const void *const block)
{
    const memory_block_guard_bytes_t *const bytes = block;
    // Check the front guard
    for (unsigned i = 0; i < ALLOCATOR_GUARD_BYTE_COUNT; ++i)
    {
        if (bytes[i] != BYTE_GUARD_IN_USE)
        {
            fprintf(stderr, "Memory block %p front guard was overwritten:\n",
                    (void *)((uintptr_t)block + ALLOCATOR_GUARD_BYTE_COUNT));
            fprintf(stderr, "\tExpected:");
            for (unsigned j = 0; j < ALLOCATOR_GUARD_BYTE_COUNT; ++j)
            {
                fprintf(stderr, " %02x", (unsigned char)BYTE_GUARD_IN_USE);
            }
            fprintf(stderr, "\n\tActual:");
            for (unsigned j = 0; j < ALLOCATOR_GUARD_BYTE_COUNT; ++j)
            {
                fprintf(stderr, " %02x", (unsigned char)bytes[i]);
            }
            fprintf(stderr, "\n");
            return 0;
        }
    }

    // Check the rear guard
    for (unsigned i = size - ALLOCATOR_GUARD_BYTE_COUNT; i < size; ++i)
    {
        if (bytes[i] != BYTE_GUARD_IN_USE)
        {
            fprintf(stderr, "Memory block %p rear guard was overwritten:\n",
                    (void *)((uintptr_t)block + ALLOCATOR_GUARD_BYTE_COUNT));
            fprintf(stderr, "\tExpected:");
            for (unsigned j = 0; j < ALLOCATOR_GUARD_BYTE_COUNT; ++j)
            {
                fprintf(stderr, " %02x", (unsigned char)BYTE_GUARD_IN_USE);
            }
            fprintf(stderr, "\n\tActual:");
            for (unsigned j = 0; j < ALLOCATOR_GUARD_BYTE_COUNT; ++j)
            {
                fprintf(stderr, " %02x", (unsigned char)bytes[i]);
            }
            fprintf(stderr, "\n");
            return 0;
        }
    }

    return 1;
}
