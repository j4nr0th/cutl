#include "fixed_size_allocator.h"
#include "allocator_internal.h"

/** Extract the memory block array from the fixed-size allocator.
 *
 * @param this Allocator from which to get the array.
 * @return Pointer to the block array.
 */
static memory_block_info_t *fixed_size_allocator_get_block_array(cutl_allocator_fs_t *const this)
{
    return (memory_block_info_t *)(this->memory);
}

static const memory_block_info_t *fixed_size_allocator_get_block_array_const(const cutl_allocator_fs_t *const this)
{
    return (const memory_block_info_t *)(this->memory);
}

static int _fixed_size_allocator_validate(const cutl_allocator_fs_t *const this)
{
    auto const memory_blocks = fixed_size_allocator_get_block_array_const(this);
    for (size_t i_block = 0; i_block < this->block_count; ++i_block)
    {
        const memory_block_info_t *const block = memory_blocks + i_block;
        // If in use, check guards are intact
        if (block->state == MEMORY_BLOCK_USED)
        {
            const int failed_guard_check =
                !_verify_block_in_use(block->size, (void *)((uintptr_t)this->memory + block->offset));
            if (failed_guard_check)
            {
                CUTL_ASSERT(!failed_guard_check, "Block %zu failed the guard check.", i_block);
                return 0;
            }
        }
        // Check that the blocks that follow it do not overlap with it.
        for (size_t j_block = i_block + 1; j_block < this->block_count; ++j_block)
        {
            const memory_block_info_t *const block_j = memory_blocks + j_block;
            // Does it have a size?
            if (block_j->size == 0)
                continue;

            // Which is in front?
            if (block_j->offset < block->offset && (block_j->offset + block_j->size > block->offset))
            {
                // block_j starts before but goes over into our block!
                CUTL_ASSERT(block_j->offset + block_j->size > block->offset, "Block %zu overlaps with block %zu.",
                            i_block, j_block);
                return 0;
            }
            if (block_j->offset > block->offset && (block_j->offset < block->offset + block->size))
            {
                CUTL_ASSERT(block_j->offset < block->offset + block->size, "Block %zu overlaps with block %zu.",
                            i_block, j_block);
                return 0;
            }
        }
    }

    size_t current_block = 0;
    // Find the block with the lowest offset
    for (size_t i_block = 0; i_block < this->block_count; ++i_block)
    {
        if (memory_blocks[i_block].offset < memory_blocks[current_block].offset)
        {
            current_block = i_block;
        }
    }

    const uintptr_t expected_top = sizeof(memory_block_info_t) * this->block_count;
    CUTL_ASSERT(memory_blocks[current_block].offset == expected_top, "Block %zu is not at the top.", current_block);

    // Check that for each block there is exactly one which starts where this one ends
    for (size_t i_checked = 0; i_checked < this->block_count - 1; ++i_checked)
    {
        size_t next_block = current_block;
        for (size_t i_block = 0; i_block < this->block_count; ++i_block)
        {
            if (memory_blocks[i_block].offset ==
                memory_blocks[current_block].offset + memory_blocks[current_block].size)
            {
                if (next_block != current_block)
                {
                    // There was another block that already met this criterion!
                    CUTL_ASSERT(next_block != current_block, "Block %zu overlaps with block %zu.", next_block, i_block);
                    return 0;
                }
                next_block = i_block;
                break;
            }
        }
        if (next_block == current_block)
        {
            // There is a gap after this block!
            CUTL_ASSERT(next_block == current_block, "Block %zu is not followed by another block.", current_block);
            return 0;
        }
        // Move to the next one
        current_block = next_block;
    }

    return 1;
}

/** Guard a memory block and return the address ready for consumption.
 *
 * @param this Allocator from which the block is extracted.
 * @param block Information about the block to guard.
 * @return Pointer to the useful region of memory.
 */
static void *make_block_guarded(const cutl_allocator_fs_t *const this, const memory_block_info_t block)
{

    auto const block_start = (uintptr_t)this->memory + block.offset;
    _prepare_block_used(block.size, (void *)block_start);
    return (void *)(block_start + ALLOCATOR_GUARD_BYTE_COUNT);
}

static void print_current_blocks(cutl_allocator_fs_t *const this)
{
    printf("Current blocks in allocator %p: %zu\nblock,offset,end,size,state\n", this, this->block_count);
    auto const block_array = fixed_size_allocator_get_block_array(this);
    for (size_t i_block = 0; i_block < this->block_count; ++i_block)
    {
        const memory_block_info_t *const block = block_array + i_block;
        const char *block_state;
        switch (block->state)
        {
        case MEMORY_BLOCK_FREE:
            block_state = "FREE";
            break;
        case MEMORY_BLOCK_USED:
            block_state = "USED";
            break;
        case MEMORY_BLOCK_MERGED:
            block_state = "MERGED";
            break;
        default:
            block_state = "UNKNOWN";
            break;
        }
        printf("%zu,%zu,%zu,%zu,%s\n", i_block, block->offset, block->offset + block->size, block->size, block_state);
    }
}

/** Allocate a new block of the required size from the fixed-size allocator.
 *
 * @param this Allocator from which to allocate the block from.
 * @param size Size of the allocation to make.
 * @param p_block Pointer which receives the index of the newly allocated block.
 * @return CUTL_SUCCESS when the allocation is possible, otherwise an appropriate error code.
 */
static cutl_result_t fixed_size_allocator_allocate_block(cutl_allocator_fs_t *const this, size_t size,
                                                         unsigned *p_block)
{
    // Check if we can expand the memory block array
    auto const block_array = fixed_size_allocator_get_block_array(this);
    const uintptr_t block_array_end_offset = sizeof(*block_array) * this->block_count;
    unsigned first_block;
    for (first_block = 0; first_block < this->block_count; ++first_block)
    {
        const memory_block_info_t *const block = block_array + first_block;
        if (block->offset == block_array_end_offset)
            break;
    }

    if (first_block == this->block_count)
    {
        print_current_blocks(this);
        CUTL_ASSERT(first_block != this->block_count, "Could not find the first block.");
        return CUTL_RESULT_FAILURE;
    }

    memory_block_info_t *const p_first = block_array + first_block;
    if (p_first->state == MEMORY_BLOCK_USED)
    {
        // We cannot allocate using this allocator anymore
        return CUTL_RESULT_INSUFFICIENT_BUFFER;
    }

    // Can the first block even fit anything anymore?
    if (p_first->size < sizeof(memory_block_info_t))
    {
        // Block is not large enough to allocate the memory from
        return CUTL_RESULT_INSUFFICIENT_BUFFER;
    }
    // Take the space from the first block for the new info entry
    p_first->size -= sizeof(memory_block_info_t);
    p_first->offset += sizeof(memory_block_info_t);

    // Round up the size needed
    size = _get_block_size(size);
    // Find a block we can use
    unsigned i_chosen = this->block_count;
    for (unsigned i_block = 0; i_block < this->block_count; ++i_block)
    {
        const memory_block_info_t *const block = block_array + i_block;
        if (block->state != MEMORY_BLOCK_FREE)
            continue;
        if (block->size < size)
            continue;

        // Take the first smallest block possible
        if (i_chosen == this->block_count || block->size < block_array[i_chosen].size)
        {
            i_chosen = i_block;
        }
    }

    if (i_chosen == this->block_count)
    {
        // Restore the state of the first block
        p_first->size += sizeof(memory_block_info_t);
        p_first->offset -= sizeof(memory_block_info_t);
        // No suitable block found
        return CUTL_RESULT_INSUFFICIENT_BUFFER;
    }

    memory_block_info_t *const p_chosen = block_array + i_chosen;
    const size_t remaining_size = p_chosen->size - size;
    if (remaining_size)
    {
        // Split the block into the (free) left half and (used) right half
        const memory_block_info_t left_block = {
            .offset = p_chosen->offset, .size = remaining_size, .state = MEMORY_BLOCK_FREE};
        const memory_block_info_t right_block = {
            .offset = left_block.offset + remaining_size, .size = size, .state = MEMORY_BLOCK_USED};
        // Update the block array
        block_array[i_chosen] = left_block;
        auto const i_new_block = this->block_count;
        block_array[i_new_block] = right_block;
        this->block_count += 1;
        *p_block = i_new_block;
    }
    else
    {
        // The whole block has to be given
        p_chosen->state = MEMORY_BLOCK_USED;
        // We can give memory back to the left block
        p_first->size += sizeof(memory_block_info_t);
        p_first->offset -= sizeof(memory_block_info_t);
        // Make sure to return the index of the block we are using
        *p_block = i_chosen;
    }

    return CUTL_SUCCESS;
}

cutl_result_t cutl_allocator_fs_allocate(cutl_allocator_fs_t *const this, const size_t size, void **p_memory)
{
    unsigned i_block;
    auto const res = fixed_size_allocator_allocate_block(this, size, &i_block);
    if (res != CUTL_SUCCESS)
        return res;

    *p_memory = make_block_guarded(this, fixed_size_allocator_get_block_array(this)[i_block]);
    return CUTL_SUCCESS;
}

/** Check if two memory blocks are next to each other (touch each other).
 *
 * @param b1 First block.
 * @param b2 Second block.
 * @return Non-zero if the two blocks touch and zero if they do not.
 */
static int memory_blocks_touch(const memory_block_info_t *b1, const memory_block_info_t *b2)
{
    const size_t b1_start = b1->offset;
    const size_t b2_start = b2->offset;
    const size_t b1_end = b1->offset + b1->size;
    const size_t b2_end = b2->offset + b2->size;
    return b1_start == b2_end || b1_end == b2_start;
}

/**
 *
 * @param block_cnt Count of blocks in the array.
 * @param blocks Array of the blocks.
 * @param i_block Index of the block that should be merged.
 * @return New number of blocks in the array.
 */
static unsigned memory_block_array_merge(const unsigned block_cnt, memory_block_info_t blocks[const static block_cnt],
                                         const unsigned i_block)
{
    unsigned merged = 0;
    // Remove zero-sized free blocks
    for (unsigned i_other = 0; i_other < block_cnt; ++i_other)
    {
        if (blocks[i_other].state == MEMORY_BLOCK_FREE && blocks[i_other].size == 0)
        {
            blocks[i_other].state = MEMORY_BLOCK_MERGED;
            merged += 1;
        }
    }

    // Merge adjacent free blocks
    for (;;)
    {
        // Check if a block
        unsigned i_other;
        // Check those before
        for (i_other = 0; i_other < i_block; ++i_other)
        {
            if (blocks[i_other].state == MEMORY_BLOCK_FREE && memory_blocks_touch(blocks + i_other, blocks + i_block))
                break;
        }
        if (i_other == i_block)
        {
            // No blocks to merge before, check after
            for (i_other = i_block + 1; i_other < block_cnt; ++i_other)
            {
                if (blocks[i_other].state == MEMORY_BLOCK_FREE &&
                    memory_blocks_touch(blocks + i_block, blocks + i_other))
                    break;
            }
        }

        if (i_other == block_cnt)
        {
            // If we have any blocks that were merged, we have to eliminate them
            if (merged)
            {
                unsigned first_merged;
                for (first_merged = 0; first_merged < block_cnt; ++first_merged)
                {
                    const memory_block_info_t *const block_r = blocks + first_merged;
                    if (block_r->state == MEMORY_BLOCK_MERGED)
                        break;
                }
                CUTL_ASSERT(first_merged != block_cnt, "Could not find the first merged block.");
                for (unsigned pos_r = first_merged + 1, pos_w = first_merged; pos_r < block_cnt; ++pos_r)
                {
                    const memory_block_info_t *const block_r = blocks + pos_r;
                    if (block_r->state == MEMORY_BLOCK_MERGED)
                        continue;
                    // Write to array
                    blocks[pos_w] = *block_r;
                    pos_w += 1;
                }
            }
            return block_cnt - merged;
        }

        // Perform the merge
        // Size is the sum
        blocks[i_block].size += blocks[i_other].size;
        // Offset is the smallest
        blocks[i_block].offset =
            blocks[i_other].offset < blocks[i_block].offset ? blocks[i_other].offset : blocks[i_block].offset;
        // The other block is now marked as merged
        blocks[i_other].state = MEMORY_BLOCK_MERGED;
        // Increase the merge count
        merged += 1;
    }
}

/** Convert the (absolute) memory address to the address relative to the allocator.
 *
 * @param this Allocator from which the address is.
 * @param memory Address to convert to relative offset from the allocator baseline.
 * @return Offset from the allocator base to the start of the block or `~(uintptr_t)0` if the address was not allocated
 * with this allocator.
 */
static uintptr_t address_to_offset(const cutl_allocator_fs_t *const this, void *const memory)
{
    // Check the memory is even in the allocator
    uintptr_t memory_ptr = (uintptr_t)memory;
    const uintptr_t allocator_mem_start = (uintptr_t)this->memory + sizeof(memory_block_info_t) * this->block_count;
    if (memory_ptr < allocator_mem_start || memory_ptr >= (uintptr_t)this->memory + this->total_size)
    {
        return ~(uintptr_t)0;
    }
    // Compensate for the guard bytes
    memory_ptr -= ALLOCATOR_GUARD_BYTE_COUNT;
    // Make memory relative to the allocator
    memory_ptr -= (uintptr_t)this->memory;
    return memory_ptr;
}

/** Find the index of the memory block, which starts at the specified relative address.
 *
 * @param block_cnt Number of blocks in the array.
 * @param blocks Array of block info structs.
 * @param address Relative address of the block to find.
 * @return Index of the block which starts with the specified relative address, or `block_cnt` if it was not found.
 */
static unsigned find_memory_block_index_by_start(const unsigned block_cnt,
                                                 const memory_block_info_t blocks[const static block_cnt],
                                                 const uintptr_t address)
{
    // Find what block it is from
    for (unsigned i_block = 0; i_block < block_cnt; ++i_block)
    {
        const memory_block_info_t *const block = blocks + i_block;
        if (block->offset == address)
        {
            return i_block;
        }
    }
    return block_cnt;
}

/** Find the index of the memory block, which ends at the specified relative address.
 *
 * @param block_cnt Number of blocks in the array.
 * @param blocks Array of block info structs.
 * @param address Relative address of the block to find.
 * @return Index of the block which ends with the specified relative address, or `block_cnt` if it was not found.
 */
static unsigned find_memory_block_index_by_end(const unsigned block_cnt,
                                               const memory_block_info_t blocks[const static block_cnt],
                                               const uintptr_t address)
{
    // Find what block it is from
    for (unsigned i_block = 0; i_block < block_cnt; ++i_block)
    {
        const memory_block_info_t *const block = blocks + i_block;
        if (block->offset + block->size == address)
        {
            return i_block;
        }
    }
    return block_cnt;
}

/** Deallocate the memory block and return it to the allocator.
 *
 * @param this Allocator to deallocate the block from.
 * @param memory_blocks Array of the allocator's memory blocks.
 * @param i_block Index of the block to deallocate.
 * @return CUTL_SUCCESS on successfully deallocated block, otherwise an appropriate error code.
 */
static cutl_result_t fixed_buffer_deallocate_block(cutl_allocator_fs_t *const this,
                                                   memory_block_info_t *const memory_blocks, const unsigned i_block)
{
    memory_block_info_t *const block = memory_blocks + i_block;
    if (block->state != MEMORY_BLOCK_USED)
    {
        return CUTL_RESULT_DOUBLE_DEALLOCATION;
    }

    // Check the block guards are set up correctly
    CUTL_ASSERT(_verify_block_in_use(block->size, (void *)((uintptr_t)this->memory + block->offset)),
                "Block guards are not set up correctly.");
    // Mark block as free
    block->state = MEMORY_BLOCK_FREE;
    const uintptr_t old_top = sizeof(memory_block_info_t) * this->block_count;
    // Merge the blocks if we can
    this->block_count = memory_block_array_merge(this->block_count, memory_blocks, i_block);
    // If we merged any blocks, then the left most block can grow.
    uintptr_t new_top = sizeof(memory_block_info_t) * this->block_count;
    if (old_top != new_top)
    {
        // Only search for the first block after we finished the merge
        const unsigned first_block = find_memory_block_index_by_start(this->block_count, memory_blocks, old_top);
        CUTL_ASSERT(first_block != this->block_count, "Could not find the first block after merge.");
        const memory_block_info_t *const first_block_ptr = memory_blocks + first_block;
        // We cannot grow the left-most block, so we must add a new free one. Space for it is guaranteed.
        if (first_block_ptr->state != MEMORY_BLOCK_FREE)
        {
            // We can add a new block
            memory_block_info_t *const new_block = memory_blocks + this->block_count;
            new_top += sizeof(memory_block_info_t);
            new_block->offset = new_top;
            new_block->size = old_top - new_top;
            new_block->state = MEMORY_BLOCK_FREE;
            this->block_count += 1;
        }
        else
        {
            // We can add the memory saved by shrinking the blocks array to the first block directly
            memory_blocks[first_block].offset = new_top;
            memory_blocks[first_block].size += old_top - new_top;
        }
    }

    return CUTL_SUCCESS;
}

cutl_result_t cutl_allocator_fs_deallocate(cutl_allocator_fs_t *const this, void *const memory)
{
    auto const memory_ptr = address_to_offset(this, memory);
    if (memory_ptr == ~(uintptr_t)0)
        return CUTL_RESULT_MISMATCHED_ALLOCATOR;

    memory_block_info_t *const memory_blocks = fixed_size_allocator_get_block_array(this);
    const unsigned i_block = find_memory_block_index_by_start(this->block_count, memory_blocks, memory_ptr);
    if (i_block == this->block_count)
    {
        // This is either an old pointer, or our block table is corrupted.
        return CUTL_RESULT_CORRUPTED_POINTER;
    }

    return fixed_buffer_deallocate_block(this, memory_blocks, i_block);
}

cutl_result_t cutl_allocator_fs_reallocate(cutl_allocator_fs_t *const this, const size_t size, void *const old_ptr,
                                           void **p_memory)
{
    // Convert the address to an allocator-relative address
    auto const memory_ptr = address_to_offset(this, old_ptr);
    if (memory_ptr == ~(uintptr_t)0)
        return CUTL_RESULT_MISMATCHED_ALLOCATOR;

    // Find the block memory belongs to.
    memory_block_info_t *const memory_blocks = fixed_size_allocator_get_block_array(this);
    const unsigned i_block = find_memory_block_index_by_start(this->block_count, memory_blocks, memory_ptr);
    if (i_block == this->block_count)
    {
        // This is either an old pointer, or our block table is corrupted.
        return CUTL_RESULT_CORRUPTED_POINTER;
    }

    // Get the real size that will be needed
    auto const real_size = _get_block_size(size);

    memory_block_info_t *const block = memory_blocks + i_block;
    // Are we good?
    if (real_size == block->size)
    {
        // We are good
        *p_memory = old_ptr;
        return CUTL_SUCCESS;
    }

    // Is the current block larger?
    if (real_size < block->size)
    {
        // We do this in three steps:
        // 1. Free this block
        // 2. Allocate a new block
        // 3. Copy memory to the new address
        // cutl_allocator_fs_deallocate(this, old_ptr);
        fixed_buffer_deallocate_block(this, memory_blocks, i_block);
        unsigned i_new_block;
        auto const res = fixed_size_allocator_allocate_block(this, size, &i_new_block);
        if (res != CUTL_SUCCESS)
        {
            CUTL_ASSERT(res != CUTL_SUCCESS,
                        "An allocation somehow failed, after we just returned a block of correct size!");
            return CUTL_RESULT_CORRUPTED_POINTER;
        }
        // Move the new memory (it may overlap with the old)
        memmove((void *)((uintptr_t)this->memory + memory_blocks[i_new_block].offset + ALLOCATOR_GUARD_BYTE_COUNT),
                old_ptr, size);
        void *const new_memory = make_block_guarded(this, memory_blocks[i_new_block]);
        *p_memory = new_memory;
        // Done
        return CUTL_SUCCESS;
    }

    // The current block is too small. If the left or right blocks are large enough and free, we can steal memory from
    // those two.
    size_t free_neighbors_size = 0;
    // Right neighbor is easy to find.
    const unsigned i_right =
        find_memory_block_index_by_start(this->block_count, memory_blocks, memory_ptr + block->size);
    const unsigned i_left = find_memory_block_index_by_end(this->block_count, memory_blocks, memory_ptr);
    if (i_right != this->block_count && memory_blocks[i_right].state == MEMORY_BLOCK_FREE)
    {
        free_neighbors_size = memory_blocks[i_right].size;
    }
    if (i_left != this->block_count && memory_blocks[i_left].state == MEMORY_BLOCK_FREE)
    {
        free_neighbors_size += memory_blocks[i_left].size;
    }

    uintptr_t needed_extra_memory = real_size - block->size;
    if (free_neighbors_size <= needed_extra_memory)
    {
        // We cannot steal from the neighbors
        // As the last option, try to allocate a totally new memory buffer
        void *mem;
        auto res = cutl_allocator_fs_allocate(this, size, &mem);
        if (res != CUTL_SUCCESS)
            return res;
        // Copy the memory
        memcpy(mem, old_ptr, size);
        // Release the current block
        res = fixed_buffer_deallocate_block(this, memory_blocks, i_block);
        CUTL_ASSERT(res == CUTL_SUCCESS, "Could not release the old memory block: (%s) - %s",
                    cutl_result_to_string(res), cutl_result_message(res));
        (void)res;
        // Return the new address
        *p_memory = mem;
        // Done
        return CUTL_SUCCESS;
    }

    // We can steal from the neighbors. First, start with the right one!
    if (i_right != this->block_count && memory_blocks[i_right].state == MEMORY_BLOCK_FREE)
    {
        memory_block_info_t *const right_block = memory_blocks + i_right;
        if (needed_extra_memory <= right_block->size)
        {
            // Right one has more than enough space!
            block->size += needed_extra_memory;
            right_block->offset += needed_extra_memory;
            right_block->size -= needed_extra_memory;
            if (right_block->size == 0)
            {
                // Get rid of the right block now that it is empty
                right_block->state = MEMORY_BLOCK_USED;
                auto const res = fixed_buffer_deallocate_block(this, memory_blocks, i_right);
                (void)res;
                CUTL_ASSERT(res == CUTL_SUCCESS, "Could not release the right memory block: (%s) - %s",
                            cutl_result_to_string(res), cutl_result_message(res));
            }
            // Set up the guards
            *p_memory = make_block_guarded(this, *block);
            // Done
            return CUTL_SUCCESS;
        }
        // We steal as much as we can from the right block
        block->size += right_block->size;
        right_block->offset += right_block->size;
        needed_extra_memory -= right_block->size;
        right_block->size = 0;
        right_block->state = MEMORY_BLOCK_USED;
        auto const res = fixed_buffer_deallocate_block(this, memory_blocks, i_right);
        (void)res;
        CUTL_ASSERT(res == CUTL_SUCCESS, "Could not release the right memory block: (%s) - %s",
                    cutl_result_to_string(res), cutl_result_message(res));
    }
    // We steal the remainder from the left neighbor
    CUTL_ASSERT(i_left != this->block_count, "Left neighbor should not be free at this point!");
    memory_block_info_t *const left_block = memory_blocks + i_left;
    left_block->size -= needed_extra_memory;
    block->offset -= needed_extra_memory;
    block->size += needed_extra_memory;
    // Move the memory
    memmove((void *)((uintptr_t)old_ptr - needed_extra_memory), old_ptr,
            block->size - 2LLU * ALLOCATOR_GUARD_BYTE_COUNT);
    // Set up the guards
    *p_memory = make_block_guarded(this, *block);
    // Done
    return CUTL_SUCCESS;
}

cutl_result_t cutl_allocator_fs_real_block_size(const cutl_allocator_fs_t *this, void *const memory,
                                                size_t *const p_size)
{
    auto const memory_ptr = address_to_offset(this, memory);
    if (memory_ptr == ~(uintptr_t)0)
        return CUTL_RESULT_MISMATCHED_ALLOCATOR;
    auto const blocks = fixed_size_allocator_get_block_array((cutl_allocator_fs_t *)this);
    const unsigned i_block = find_memory_block_index_by_start(this->block_count, blocks, memory_ptr);
    if (i_block == this->block_count)
        return CUTL_RESULT_CORRUPTED_POINTER;

    auto const block = blocks + i_block;
    *p_size = block->size - 2LLU * ALLOCATOR_GUARD_BYTE_COUNT;
    return CUTL_SUCCESS;
}

/**
 * Wraps fixed-size allocator for the `cutl_allocator_t` interface.
 */
static void *fixed_size_allocator_wrap_allocate(void *state, const size_t size)
{
    cutl_allocator_fs_t *const allocator = state;
    void *memory;
    const cutl_result_t res = cutl_allocator_fs_allocate(allocator, size, &memory);
    if (res != CUTL_SUCCESS)
    {
        return nullptr;
    }
    CUTL_ASSERT(_fixed_size_allocator_validate(allocator), "Failed validation!");
    return memory;
}

/**
 * Wraps fixed-size allocator for the `cutl_allocator_t` interface.
 */
static void fixed_size_allocator_wrap_deallocate(void *state, void *const ptr)
{
    cutl_allocator_fs_t *const allocator = state;

    const cutl_result_t res = cutl_allocator_fs_deallocate(allocator, ptr);
    CUTL_ASSERT(res == CUTL_SUCCESS, "Could not deallocate memory: (%s) - %s", cutl_result_to_string(res),
                cutl_result_message(res));
    CUTL_ASSERT(_fixed_size_allocator_validate(allocator), "Failed validation!");
}

/**
 * Wraps fixed-size allocator for the `cutl_allocator_t` interface.
 */
static void *fixed_size_allocator_wrap_realloc(void *state, void *old_ptr, const size_t size)
{
    cutl_allocator_fs_t *const allocator = state;

    void *memory;
    const cutl_result_t res = cutl_allocator_fs_reallocate(allocator, size, old_ptr, &memory);
    if (res != CUTL_SUCCESS)
    {
        return nullptr;
    }
    CUTL_ASSERT(_fixed_size_allocator_validate(allocator), "Failed validation!");
    return memory;
}

cutl_result_t cutl_allocator_fs_create(size_t size, unsigned char CUTL_ARRAY_ARG(memory, const size),
                                       cutl_allocator_fs_t **p_allocator)
{
    if (!_check_alignment(memory))
        return CUTL_RESULT_INSUFFICIENT_ALIGNMENT;

    // Make sure the size rounded down to the correct alignment
    size = _round_align_floor(size);

    if (size < sizeof(cutl_allocator_fs_t))
        return CUTL_RESULT_INSUFFICIENT_BUFFER;

    cutl_allocator_fs_t *const allocator = (cutl_allocator_fs_t *)memory;
    allocator->block_count = 1;
    allocator->total_size = size - sizeof(cutl_allocator_fs_t);
    allocator->base = (cutl_allocator_t){
        .state = allocator,
        .allocate = fixed_size_allocator_wrap_allocate,
        .deallocate = fixed_size_allocator_wrap_deallocate,
        .reallocate = fixed_size_allocator_wrap_realloc,
    };
    *p_allocator = allocator;
    // Create the first block
    fixed_size_allocator_get_block_array(allocator)[0] = (memory_block_info_t){
        .size = allocator->total_size - sizeof(memory_block_info_t), .offset = sizeof(memory_block_info_t)};
    return CUTL_SUCCESS;
}

const cutl_allocator_t *cutl_allocator_fs_get(const cutl_allocator_fs_t *this)
{
    return &this->base;
}