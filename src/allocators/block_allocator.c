
#include "block_allocator.h"

#include "allocator_internal.h"
// Counters for blocks must come in units of `ALLOCATOR_MINIMUM_ALIGNMENT` bytes.
auto constexpr blocks_per_counter_unit = ALLOCATOR_MINIMUM_ALIGNMENT * 8LLU;

static uintptr_t block_offset(const cutl_allocator_block_t *const this, const unsigned block_idx)
{
    auto const offset_counters =
        (this->block_count + blocks_per_counter_unit - 1) / blocks_per_counter_unit * ALLOCATOR_MINIMUM_ALIGNMENT;
    return offset_counters + (uintptr_t)block_idx * this->block_size;
}

static bool block_get_state(const cutl_allocator_block_t *const this, const unsigned block_idx)
{
    CUTL_ASSERT(block_idx < this->block_count, "Block index %u is out of bounds.", block_idx);
    return (bool)(this->memory[block_idx / 8] & (1 << (block_idx % 8)));
}

static void block_set_state(cutl_allocator_block_t *const this, const unsigned block_idx, const bool free)
{
    CUTL_ASSERT(block_idx < this->block_count, "Block index %u is out of bounds.", block_idx);
    auto const mask = (unsigned char)(1 << (block_idx % 8));
    auto const ptr = this->memory + (block_idx / 8);
    if (free)
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

        // We can grab the lowest one
        block_idx = i * 8;
        // Get the lowest bit in isolation
        auto lowest_byte = group_state & ~(group_state - 1);
        // While the lowest bit is still there, we shift down
        while (lowest_byte)
        {
            block_idx += 1;
            lowest_byte >>= 1;
        }
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
    const auto offset = address_to_offset(this, memory);
    if (offset == ~(uintptr_t)0 || offset < ALLOCATOR_GUARD_BYTE_COUNT)
        return CUTL_RESULT_MISMATCHED_ALLOCATOR;

    // Is the requested size too large?
    if (_get_block_size(size) > this->block_size)
        return CUTL_RESULT_OUT_OF_MEMORY;

    // Ok, we are done now
    *p_memory = memory;
    return CUTL_SUCCESS;
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
    block_size += 2 * ALLOCATOR_GUARD_BYTE_COUNT;

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