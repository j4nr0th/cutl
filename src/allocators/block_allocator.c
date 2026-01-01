#include "../../include/cutl/allocators/block_allocator.h"
#include "allocator_internal.h"
#include <stdbit.h>

struct cutl_allocator_block_t
{
    cutl_allocator_t base;                       // Allocator interface
    unsigned block_size;                         // Size of individual blocks
    unsigned block_count;                        // Number of blocks in the allocator
    alignas(max_align_t) unsigned char memory[]; // Memory used to back the allocations and store the block states
};

// Counters for blocks must come in units of `ALLOCATOR_MINIMUM_ALIGNMENT` bytes.
auto constexpr blocks_per_counter_unit = ALLOCATOR_MINIMUM_ALIGNMENT * 8LLU;

/**
 * Compute the offset of the block's start in the allocator.
 *
 * @param this Allocator to use.
 * @param block_idx Index of the block to get the offset for.
 * @return Offset of the block's start from this->memory.
 */
static uintptr_t block_offset(const cutl_allocator_block_t *const this, const unsigned block_idx)
{
    auto const offset_counters =
        (this->block_count + blocks_per_counter_unit - 1) / blocks_per_counter_unit * ALLOCATOR_MINIMUM_ALIGNMENT;
    return offset_counters + (uintptr_t)block_idx * this->block_size;
}

/**
 * Get the state of the block in the allocator.
 *
 * @param this Allocator to use.
 * @param block_idx Index of the block to get the state for.
 * @return If the block is in use, `true` is returned, and if it is free, `false` is returned.
 */
static bool block_get_state(const cutl_allocator_block_t *const this, const unsigned block_idx)
{
    CUTL_ASSERT(block_idx < this->block_count, "Block index %u is out of bounds.", block_idx);
    return (bool)(this->memory[block_idx / 8] & (1 << (block_idx % 8)));
}

// static void print_block_info(const cutl_allocator_block_t *const this)
// {
//     printf("Block allocator with %u blocks of size %u\n", this->block_count, this->block_size);
//     printf("Block counters:\n");
//     for (unsigned i = 0; i < this->block_count; ++i)
//     {
//         auto const block_state = block_get_state(this, i);
//         printf("%c", block_state ? 'X' : '.');
//         if ((i + 1) % 8 == 0)
//             printf("\n");
//     }
// }

/**
 * Set the block as used/free.
 *
 * @param this Allocator to use.
 * @param block_idx Index of the block to set the state for.
 * @param new_state The new state of the block (`true` for used and `false` for free).
 */
static void block_set_state(cutl_allocator_block_t *const this, const unsigned block_idx, const bool new_state)
{
    CUTL_ASSERT(block_idx < this->block_count, "Block index %u is out of bounds.", block_idx);
    auto const mask = (unsigned char)(1 << (block_idx % 8));
    auto const ptr = this->memory + (block_idx / 8);
    if (!new_state)
    {
        CUTL_ASSERT(*ptr & mask, "Block %u is already free.", block_idx);
        *ptr &= ~mask; // Remove the block bit
    }
    else
    {
        CUTL_ASSERT(!(*ptr & mask), "Block %u is already not free.", block_idx);
        *ptr |= mask; // Add the block bit
    }
}

cutl_result_t cutl_allocator_block_allocate(cutl_allocator_block_t *const this, void **const p_memory)
{
    // Get the first free block
    auto block_idx = 0U;
    auto i = 0U;
    auto const counter_units = (this->block_count + 7) / 8;
    for (i = 0; i < counter_units; ++i)
    {
        auto const group_state = this->memory[i];
        // This group of 8 is all used up
        if (group_state == 255)
            continue;

        // We can grab the lowest free one
        block_idx = i * 8;
        auto const first_trailing_zero = stdc_first_trailing_zero_uc(group_state) - 1;
        CUTL_ASSUME(first_trailing_zero < 8 * sizeof(group_state));
        block_idx += first_trailing_zero;
        // // Get the lowest free bit in isolation
        // auto lowest_byte = (~group_state & ~(~group_state - 1));
        // // While the lowest bit is still there, we shift down
        // while (lowest_byte > 1)
        // {
        //     block_idx += 1;
        //     lowest_byte >>= 1;
        // }
        break;
    }
    // We did not find a free block
    if (i == counter_units || block_idx >= this->block_count)
        return CUTL_RESULT_OUT_OF_MEMORY;

    // Assert the block is free
    CUTL_ASSERT(block_get_state(this, block_idx) == 0, "Block %u was not free!", block_idx);

    // Mark the block as used
    block_set_state(this, block_idx, true);
    // Set up the block guards
    auto const ptr_block = this->memory + block_offset(this, block_idx);
    _prepare_block_used(this->block_size, ptr_block);
    // Return the memory pointer
    *p_memory = (void *)(ptr_block + ALLOCATOR_GUARD_BYTE_COUNT);

    return CUTL_SUCCESS;
}

/**
 * Convert the absolute address of a block from the block allocator to a relative offset.
 *
 * @param this Allocator to compute the offset relative to.
 * @param memory Absolute memory address to convert to the relative offset.
 * @return Offset of the block's start from this->memory or `~(uintptr_t)0` if the memory is not from the allocator.
 */
static uintptr_t address_to_offset(const cutl_allocator_block_t *const this, const void *const memory)
{
    auto const address = (uintptr_t)memory;
    auto const counter_bytes =
        (this->block_count + blocks_per_counter_unit - 1) / blocks_per_counter_unit * ALLOCATOR_MINIMUM_ALIGNMENT;
    auto const blocks_start = (uintptr_t)this->memory + counter_bytes;
    // Make sure we are not out of bounds
    if (address < blocks_start || address >= blocks_start + (uintptr_t)this->block_count * this->block_size)
        return ~(uintptr_t)0;

    return address - blocks_start;
}

cutl_result_t cutl_allocator_block_deallocate(cutl_allocator_block_t *const this, void *const memory)
{
    // Check if the block can even be from this allocator
    auto offset = address_to_offset(this, memory);
    if (offset == ~(uintptr_t)0 || offset < ALLOCATOR_GUARD_BYTE_COUNT)
        return CUTL_RESULT_MISMATCHED_ALLOCATOR;

    // Adjust the offset to account for guard bytes
    offset -= ALLOCATOR_GUARD_BYTE_COUNT;
    // Offset should be a multiple of block size
    if (offset % this->block_size != 0)
        return CUTL_RESULT_CORRUPTED_POINTER;
    // Get the block index
    auto const block_idx = offset / this->block_size;
    // Assert the block is free
    CUTL_ASSERT(block_get_state(this, block_idx) == 1, "Block %zu was not free!", block_idx);
    // Mark the block as free
    block_set_state(this, block_idx, false);
    return CUTL_SUCCESS;
}

cutl_result_t cutl_allocator_block_reallocate(cutl_allocator_block_t *const this, void *const memory, const size_t size,
                                              void **const p_memory)
{
    // Just check that the memory passed to the function was from this allocator
    auto const offset = address_to_offset(this, memory);
    if (offset == ~(uintptr_t)0 || offset < ALLOCATOR_GUARD_BYTE_COUNT)
        return CUTL_RESULT_MISMATCHED_ALLOCATOR;

    // Is the requested size too large?
    if (_get_block_size(size) > this->block_size)
        return CUTL_RESULT_OUT_OF_MEMORY;

    // Ok, we are done now
    *p_memory = memory;
    return CUTL_SUCCESS;
}
unsigned cutl_allocator_block_get_block_size(const cutl_allocator_block_t *this)
{
    return this->block_size - 2LLU * ALLOCATOR_GUARD_BYTE_COUNT;
}

static void *wrap_allocate(void *state, const size_t size)
{
    auto const allocator = (cutl_allocator_block_t *)state;
    if (size == 0 || _get_block_size(size) > allocator->block_size)
        return nullptr;
    void *memory;
    auto const res = cutl_allocator_block_allocate(allocator, &memory);
    if (res != CUTL_SUCCESS)
    {
        CUTL_ASSERT(res == CUTL_RESULT_OUT_OF_MEMORY, "Could not allocate memory: (%s) - %s",
                    cutl_result_to_string(res), cutl_result_message(res));
        return nullptr;
    }
    return memory;
}

static void *wrap_reallocate(void *state, void *memory, const size_t new_size)
{
    auto const allocator = (cutl_allocator_block_t *)state;
    if (memory == nullptr)
    {
        return wrap_allocate(state, new_size);
    }
    if (new_size == 0)
    {
        auto const res = cutl_allocator_block_deallocate(allocator, memory);
        CUTL_ASSERT(res == CUTL_SUCCESS, "Could not deallocate memory: (%s) - %s", cutl_result_to_string(res),
                    cutl_result_message(res));
        return nullptr;
    }

    if (_get_block_size(new_size) > allocator->block_size)
        return nullptr;

    auto const res = cutl_allocator_block_reallocate(allocator, memory, new_size, &memory);
    if (res != CUTL_SUCCESS)
    {
        CUTL_ASSERT(res == CUTL_RESULT_OUT_OF_MEMORY, "Could not reallocate memory: (%s) - %s",
                    cutl_result_to_string(res), cutl_result_message(res));
        return nullptr;
    }
    return memory;
}

static void wrap_deallocate(void *state, void *memory)
{
    auto const allocator = (cutl_allocator_block_t *)state;
    auto const res = cutl_allocator_block_deallocate(allocator, memory);
    CUTL_ASSERT(res == CUTL_SUCCESS, "Could not deallocate memory: (%s) - %s", cutl_result_to_string(res),
                cutl_result_message(res));
}

cutl_result_t cutl_allocator_block_create(const size_t size, unsigned char CUTL_ARRAY_ARG(memory, size),
                                          unsigned block_size, cutl_allocator_block_t **const p_allocator)
{
    // Check we are properly aligned
    if (!_check_alignment(memory))
        return CUTL_RESULT_INSUFFICIENT_ALIGNMENT;

    // Adjust the block size to also hold the padding bytes
    block_size = _get_block_size(block_size);

    // Can we use at least one block and the array holding the block info?
    auto useful_size = size - sizeof(cutl_allocator_block_t);
    if (useful_size < 2LLU * block_size)
        return CUTL_RESULT_INSUFFICIENT_BUFFER;

    // First, compute the upper bound for block count
    auto n_blocks = useful_size / block_size;
    // Based on the upper bound, we compute the number of units of counters needed
    auto const n_counters = (n_blocks + blocks_per_counter_unit - 1) / blocks_per_counter_unit;
    auto const counter_byte_count = n_counters * ALLOCATOR_MINIMUM_ALIGNMENT;
    // Subtract the memory needed for block counters
    useful_size -= counter_byte_count;
    // Adjust the block number
    n_blocks = useful_size / block_size;

    // Fill the allocator data
    auto const allocator = (cutl_allocator_block_t *)memory;
    allocator->block_size = block_size;
    allocator->block_count = n_blocks;
    allocator->base = (cutl_allocator_t){
        .state = allocator,
        .allocate = wrap_allocate,
        .deallocate = wrap_deallocate,
        .reallocate = wrap_reallocate,
    };
    // Clear the state of the block counters
    memset(allocator->memory, 0, counter_byte_count);

    // Assert they are all free
    for (unsigned i = 0; i < n_blocks; ++i)
    {
        CUTL_ASSERT(block_get_state(allocator, i) == false, "Block %u is not free.", i);
    }

    *p_allocator = allocator;
    return CUTL_SUCCESS;
}

const cutl_allocator_t *cutl_allocator_block_get(cutl_allocator_block_t *this)
{
    return &this->base;
}