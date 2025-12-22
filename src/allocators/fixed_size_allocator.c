#include "fixed_size_allocator.h"
#include "allocator_internal.h"

//  only for profiling
#define static

/** Get the array of used memory blocks.
 *
 * @param this Allocator from which to get the array.
 * @return Pointer to the block array.
 */
static memory_block_info_t *fixed_size_allocator_get_all_blocks(const cutl_allocator_fs_t *const this)
{
    return (memory_block_info_t *)(this->memory);
}

/** Get the array of free blocks.
 *
 * @param this Allocator from which to get the array.
 * @return Pointer to the block array.
 */
static memory_block_info_t *fixed_size_allocator_get_free_blocks(const cutl_allocator_fs_t *const this)
{
    return (memory_block_info_t *)(this->memory) + (this->block_count - this->free_blocks);
}

static int _fixed_size_allocator_validate(const cutl_allocator_fs_t *const this)
{
#ifdef CUTL_VALIDATE_ALLOCATORS
    auto const memory_blocks = fixed_size_allocator_get_all_blocks(this);
    for (size_t i_block = 0; i_block < this->block_count; ++i_block)
    {
        const memory_block_info_t *const block = memory_blocks + i_block;
        // Check the used are ahead of free
        if (i_block < this->block_count - this->free_blocks)
        {
            if (block->state != MEMORY_BLOCK_USED)
            {
                CUTL_ASSERT(0, "Block %zu is not used.", i_block);
                return 0;
            }
        }
        else
        {
            if (block->state != MEMORY_BLOCK_FREE)
            {
                CUTL_ASSERT(0, "Block %zu is not free.", i_block);
                return 0;
            }
        }

        // If in use, check guards are intact
        if (block->state == MEMORY_BLOCK_USED)
        {
            const int failed_guard_check =
                !_verify_block_in_use(block->size, (void *)((uintptr_t)this->memory + block->offset));
            if (failed_guard_check)
            {
                CUTL_ASSERT(0, "Block %zu failed the guard check.", i_block);
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
                CUTL_ASSERT(0, "Block %zu overlaps with block %zu.", i_block, j_block);
                return 0;
            }
            if (block_j->offset > block->offset && (block_j->offset < block->offset + block->size))
            {
                CUTL_ASSERT(0, "Block %zu overlaps with block %zu.", i_block, j_block);
                return 0;
            }
        }
    }

    // Find the block with the lowest offset
    size_t current_block = this->block_count - 1;

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
                    CUTL_ASSERT(0, "Block %zu overlaps with block %zu.", next_block, i_block);
                    return 0;
                }
                next_block = i_block;
                break;
            }
        }
        if (next_block == current_block)
        {
            // There is a gap after this block!
            CUTL_ASSERT(0, "Block %zu is not followed by another block.", current_block);
            return 0;
        }
        // Move to the next one
        current_block = next_block;
    }
#endif // CUTL_VALIDATE_ALLOCATORS
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
    printf("Current blocks in allocator %p: %zu (%zu free)\nblock,offset,end,size,state\n", this, this->block_count,
           this->free_blocks);
    auto const block_array = fixed_size_allocator_get_all_blocks(this);
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
 * @param can_defragment When non-zero, we can run a defragmentation run to attempt and recover enough memory
 * for the allocaiton.
 * @return CUTL_SUCCESS when the allocation is possible, otherwise an appropriate error code.
 */
static cutl_result_t fixed_size_allocator_allocate_block(cutl_allocator_fs_t *const this, const size_t size,
                                                         unsigned *p_block, const int can_defragment)
{
    // Check if we can expand the memory block array
    auto const free_block_array = fixed_size_allocator_get_free_blocks(this);

    const unsigned first_block = this->free_blocks - 1;
    memory_block_info_t *const p_first = free_block_array + first_block;
    if (p_first->state == MEMORY_BLOCK_USED)
    {
        // We cannot allocate using this allocator anymore
        return CUTL_RESULT_OUT_OF_MEMORY;
    }

    // Can the first block even fit anything anymore?
    if (p_first->size < sizeof(memory_block_info_t))
    {
        // Block is not large enough to allocate the memory from
        return CUTL_RESULT_OUT_OF_MEMORY;
    }
    // Take the space from the first block for the new info entry
    p_first->size -= sizeof(memory_block_info_t);
    p_first->offset += sizeof(memory_block_info_t);

    // Round up the size needed
    auto const effective_size = _get_block_size(size);
    // Find a block we can use
    unsigned i_chosen = this->block_count;
    for (unsigned i_block = 0; i_block < this->free_blocks; ++i_block)
    {
        const memory_block_info_t *const block = free_block_array + i_block;

        if (block->size < effective_size)
            continue;

        // Take the first smallest block possible
        if (i_chosen == this->block_count || block->size < free_block_array[i_chosen].size)
        {
            i_chosen = i_block;
        }
    }

    if (i_chosen == this->free_blocks)
    {
        // Restore the state of the first block
        p_first->size += sizeof(memory_block_info_t);
        p_first->offset -= sizeof(memory_block_info_t);
        // No suitable block found

        if (can_defragment)
        {
            // Defragment
            cutl_allocator_fs_defragment(this);

            // Try again, but now without an option to defragment
            return fixed_size_allocator_allocate_block(this, size, p_block, 0);
        }

        return CUTL_RESULT_INSUFFICIENT_BUFFER;
    }

    memory_block_info_t *const p_chosen = free_block_array + i_chosen;
    const size_t remaining_size = p_chosen->size - effective_size;
    if (remaining_size > 2LLU * ALLOCATOR_GUARD_BYTE_COUNT || i_chosen == first_block)
    {
        // Split the block into the (free) left half and (used) right half
        const memory_block_info_t new_block = {
            .offset = p_chosen->offset + remaining_size, .size = effective_size, .state = MEMORY_BLOCK_USED};
        // Reduce the size of the free memory block
        p_chosen->size = remaining_size;
        // Move the first block one place forward to make space for the new one
        free_block_array[this->free_blocks] = free_block_array[this->free_blocks - 1];
        // Move the first free block out of the way
        free_block_array[this->free_blocks - 1] = free_block_array[0];
        // Place the new full block where the first free block was
        free_block_array[0] = new_block;
        this->block_count += 1;
    }
    else
    {
        this->free_blocks -= 1;
        // The whole block has to be given
        p_chosen->state = MEMORY_BLOCK_USED;
        // We can give memory back to the left block
        p_first->size += sizeof(memory_block_info_t);
        p_first->offset -= sizeof(memory_block_info_t);
        // Swap the first free block with the one we chose
        const memory_block_info_t tmp = free_block_array[0];
        free_block_array[0] = free_block_array[i_chosen];
        free_block_array[i_chosen] = tmp;
        // Make sure to return the index of the block we are using
    }
    *p_block = this->block_count - this->free_blocks - 1;

    return CUTL_SUCCESS;
}

cutl_result_t cutl_allocator_fs_allocate(cutl_allocator_fs_t *const this, const size_t size, void **p_memory)
{
    unsigned i_block;
    auto const res = fixed_size_allocator_allocate_block(this, size, &i_block, 1);
    if (res != CUTL_SUCCESS)
        return res;

    *p_memory = make_block_guarded(this, fixed_size_allocator_get_all_blocks(this)[i_block]);
    return CUTL_SUCCESS;
}

/**
 * This function should only be called on arrays with all free blocks.
 *
 * @param block_cnt Count of blocks in the array.
 * @param blocks Array of free blocks.
 * @return Number of blocks that were merged.
 */
static unsigned memory_block_array_defragment(unsigned block_cnt, memory_block_info_t blocks[const static block_cnt])
{
    unsigned merged = 0;
    // Remove zero-sized free blocks
    for (unsigned i_other = 0; i_other < block_cnt; ++i_other)
    {
        if (blocks[i_other].size == 0)
        {
            merged += 1;
        }
        else if (merged != 0)
        {
            blocks[i_other - merged] = blocks[i_other];
        }
    }

    // Update the new number of blocks
    block_cnt -= merged;

    // Merge adjacent free blocks
    // The goal is to sort the array based on descending offset and merge adjacent blocks in the process
    unsigned eliminated = 0;
    for (unsigned i_pos = 0; i_pos < block_cnt - 1; ++i_pos)
    {
        // Skip a merged block
        if (blocks[i_pos].state == MEMORY_BLOCK_MERGED)
            continue;

        for (unsigned i_other = i_pos + 1; i_other < block_cnt; ++i_other)
        {
            // Was it already merged?
            if (blocks[i_other].state == MEMORY_BLOCK_MERGED)
                continue;

            // Should we swap the blocks (not sorted)?
            if (blocks[i_other].offset > blocks[i_pos].offset)
            {
                // Swap the blocks
                const memory_block_info_t tmp = blocks[i_pos];
                blocks[i_pos] = blocks[i_other];
                blocks[i_other] = tmp;
                // Redo this iteration
                i_other = i_pos;
                continue;
            }

            // Are the two blocks one after another?
            if (blocks[i_other].offset + blocks[i_other].size != blocks[i_pos].offset)
            {
                continue;
            }

            // Merge the two blocks
            blocks[i_pos].size += blocks[i_other].size;
            blocks[i_pos].offset = blocks[i_other].offset;
            blocks[i_other].state = MEMORY_BLOCK_MERGED;
            eliminated += 1;
        }
    }

    // Remove the merged blocks from the array now
    if (eliminated)
    {
        for (unsigned i_read = 0, i_write = 0; i_read < block_cnt; ++i_read)
        {
            if (blocks[i_read].state != MEMORY_BLOCK_MERGED)
            {
                blocks[i_write] = blocks[i_read];
                i_write += 1;
            }
        }
        block_cnt -= eliminated;

        // Now we have a sorted array of free blocks, which means that we can go back to front and merge them again
        for (unsigned i = block_cnt; i > 0; --i)
        {
            auto const next_block = blocks + (i - 1);
            auto const this_block = blocks + i;
            if (next_block->offset != this_block->offset + this_block->size)
            {
                // Cannot merge these
                continue;
            }
            next_block->offset = this_block->offset;
            next_block->size += this_block->size;
            eliminated += 1;
        }
    }

    return merged + eliminated;
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

// TODO: currently the slowest part of the allocator is `fixed_buffer_deallocate_block`, so that can be sped up

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
    // Move the block to the free blocks region by swapping the last used block with the first free block
    auto const tmp = memory_blocks[this->block_count - this->free_blocks - 1];
    memory_blocks[this->block_count - this->free_blocks - 1] = *block;
    *block = tmp;
    this->free_blocks += 1;

    if (this->block_count == this->free_blocks)
    {
        // All blocks that exist are free. This means we can just reset the allocator to have one huge block.
        memory_blocks[0] = (memory_block_info_t){
            .size = this->total_size - sizeof(memory_block_info_t),
            .offset = sizeof(memory_block_info_t),
            .state = MEMORY_BLOCK_FREE,
        };
        this->block_count = 1;
        this->free_blocks = 1;
    }

    return CUTL_SUCCESS;
}

cutl_result_t cutl_allocator_fs_deallocate(cutl_allocator_fs_t *const this, void *const memory)
{
    auto const memory_ptr = address_to_offset(this, memory);
    if (memory_ptr == ~(uintptr_t)0)
        return CUTL_RESULT_MISMATCHED_ALLOCATOR;

    memory_block_info_t *const memory_blocks = fixed_size_allocator_get_all_blocks(this);
    const unsigned i_block =
        find_memory_block_index_by_start(this->block_count - this->free_blocks, memory_blocks, memory_ptr);
    if (i_block == this->block_count - this->free_blocks)
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
    memory_block_info_t *const all_memory_blocks = fixed_size_allocator_get_all_blocks(this);
    const unsigned i_block =
        find_memory_block_index_by_start(this->block_count - this->free_blocks, all_memory_blocks, memory_ptr);
    if (i_block == this->block_count - this->free_blocks)
    {
        // This is either an old pointer, or our block table is corrupted.
        return CUTL_RESULT_CORRUPTED_POINTER;
    }

    // Get the real size that will be needed
    auto const real_size = _get_block_size(size);

    memory_block_info_t *block = all_memory_blocks + i_block;
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
        fixed_buffer_deallocate_block(this, all_memory_blocks, i_block);
        unsigned i_new_block;
        auto const res = fixed_size_allocator_allocate_block(this, size, &i_new_block, 1);
        if (res != CUTL_SUCCESS)
        {
            CUTL_ASSERT(res != CUTL_SUCCESS,
                        "An allocation somehow failed, after we just returned a block of correct size!");
            return CUTL_RESULT_CORRUPTED_POINTER;
        }
        // Move the new memory (it may overlap with the old)
        memmove((void *)((uintptr_t)this->memory + all_memory_blocks[i_new_block].offset + ALLOCATOR_GUARD_BYTE_COUNT),
                old_ptr, size);
        void *const new_memory = make_block_guarded(this, all_memory_blocks[i_new_block]);
        *p_memory = new_memory;
        // Done
        return CUTL_SUCCESS;
    }

    auto const free_blocks = fixed_size_allocator_get_free_blocks(this);
    // The current block is too small. If the left or right blocks are large enough and free, we can steal memory from
    // those two.
    const unsigned i_right = find_memory_block_index_by_start(this->free_blocks, free_blocks, memory_ptr + block->size);

    const uintptr_t needed_extra_memory = real_size - block->size;
    if (i_right == this->free_blocks ||                    // Right block does not even exist
        free_blocks[i_right].state != MEMORY_BLOCK_FREE || // The right block is not free
        free_blocks[i_right].size <= needed_extra_memory   // The right block is too small
    )
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
        res = fixed_buffer_deallocate_block(this, all_memory_blocks, i_block);
        CUTL_ASSERT(res == CUTL_SUCCESS, "Could not release the old memory block: (%s) - %s",
                    cutl_result_to_string(res), cutl_result_message(res));
        (void)res;
        // Return the new address
        *p_memory = mem;
        // Done
        return CUTL_SUCCESS;
    }

    // We can steal from the right neighbor
    memory_block_info_t *const right_block = free_blocks + i_right;
    // Right one has more than enough space!
    block->size += needed_extra_memory;
    right_block->offset += needed_extra_memory;
    right_block->size -= needed_extra_memory;
    if (right_block->size == 0)
    {
        // Get rid of the right block now that it is empty
        right_block->state = MEMORY_BLOCK_USED;
        auto const res = fixed_buffer_deallocate_block(this, all_memory_blocks, i_right);
        (void)res;
        CUTL_ASSERT(res == CUTL_SUCCESS, "Could not release the right memory block: (%s) - %s",
                    cutl_result_to_string(res), cutl_result_message(res));
    }
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
    auto const blocks = fixed_size_allocator_get_all_blocks((cutl_allocator_fs_t *)this);
    const unsigned i_block =
        find_memory_block_index_by_start(this->block_count - this->free_blocks, blocks, memory_ptr);
    if (i_block == this->block_count - this->free_blocks)
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
    // cutl_allocator_fs_defragment(allocator);
    CUTL_ASSERT(_fixed_size_allocator_validate(allocator), "Failed validation!");
    const cutl_result_t res = cutl_allocator_fs_allocate(allocator, size, &memory);
    CUTL_ASSERT(_fixed_size_allocator_validate(allocator), "Failed validation!");
    if (res != CUTL_SUCCESS)
    {
        return nullptr;
    }
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

    auto const allocator = (cutl_allocator_fs_t *)memory;
    allocator->block_count = 1;
    allocator->free_blocks = 1;
    allocator->total_size = size - sizeof(cutl_allocator_fs_t);
    allocator->base = (cutl_allocator_t){
        .state = allocator,
        .allocate = fixed_size_allocator_wrap_allocate,
        .deallocate = fixed_size_allocator_wrap_deallocate,
        .reallocate = fixed_size_allocator_wrap_realloc,
    };
    *p_allocator = allocator;
    // Create the first block
    fixed_size_allocator_get_all_blocks(allocator)[0] = (memory_block_info_t){
        .size = allocator->total_size - sizeof(memory_block_info_t),
        .offset = sizeof(memory_block_info_t),
        .state = MEMORY_BLOCK_FREE,
    };
    return CUTL_SUCCESS;
}

void cutl_allocator_fs_defragment(cutl_allocator_fs_t *this)
{
    if (this->free_blocks == 1)
        return;

    auto const free_blocks = fixed_size_allocator_get_free_blocks(this);
    auto const merged = memory_block_array_defragment(this->free_blocks, free_blocks);
    this->free_blocks -= merged;
    this->block_count -= merged;
    auto const freed_memory = merged * sizeof(memory_block_info_t);
    auto const first_block = free_blocks + (this->free_blocks - 1);
    CUTL_ASSERT(first_block->state == MEMORY_BLOCK_FREE, "First block is not free!");
    first_block->size += freed_memory;
    first_block->offset -= freed_memory;
}

const cutl_allocator_t *cutl_allocator_fs_get(const cutl_allocator_fs_t *this)
{
    return &this->base;
}