#include "../../include/cutl/allocators/stack_allocator.h"
#include "allocator_internal.h"

struct cutl_allocator_stack_t
{
    cutl_allocator_t base;
    size_t top;                                        // Top of the stack
    size_t size;                                       // Total size of the stack
    alignas(ALLOCATOR_ALIGNMENT) unsigned char data[]; // Remaining memory
};

typedef struct
{
    size_t size;                // Size of the allocation
    memory_block_state_t state; // State of the allocation
} stack_block_info_t;

// The bookkeeping is padded to the allocation alignment, as it sits in front of the memory handed to
// the caller. Without that padding, a block would not line up with the alignment the allocator
// promises whenever the bookkeeping and the front guard do not add up to a multiple of it.
enum
{
    STACK_BLOCK_INFO_SIZE = CUTL_ALLOCATOR_ROUND_UP(sizeof(stack_block_info_t)),
};

cutl_result_t cutl_allocator_stack_allocate(cutl_allocator_stack_t *const this, size_t size, void **const p_memory)
{
    // Round the size up
    size = allocator_block_size(size + 2 * STACK_BLOCK_INFO_SIZE);
    // Check if we are over the top of the stack yet
    if (this->top + size > this->size)
        return CUTL_RESULT_OUT_OF_MEMORY;

    // Get the address from the top of the stack
    auto const start_address = (uintptr_t)this->data + this->top;
    *p_memory = (void *)(start_address + STACK_BLOCK_INFO_SIZE + allocator_user_offset());
    this->top += size;
    // Set up block guards
    allocator_set_guards((void *)(start_address + STACK_BLOCK_INFO_SIZE), size - 2 * STACK_BLOCK_INFO_SIZE);
    // Set the end of block information
    auto const end_address = (stack_block_info_t *)(start_address + size - STACK_BLOCK_INFO_SIZE);
    end_address->state = MEMORY_BLOCK_USED; // State is obviously used
    end_address->size = size;               // Real size
    // Set the start of block information
    auto const start_address_info = (stack_block_info_t *)(start_address);
    start_address_info->state = MEMORY_BLOCK_USED;
    start_address_info->size = size;

    return CUTL_SUCCESS;
}

static uintptr_t validate_and_adjust_memory_address(const cutl_allocator_stack_t *const this, void *const memory)
{
    auto real_address = (uintptr_t)memory;
    // Check the block is from the allocator
    if (real_address < (uintptr_t)this->data || real_address >= (uintptr_t)(this->data + this->size))
    {
        return ~(uintptr_t)0;
    }

    // Adjust with the offset of the guard bytes
    real_address -= allocator_user_offset() + STACK_BLOCK_INFO_SIZE;
    return real_address;
}

static cutl_result_t stack_memory_info_blocks(const uintptr_t real_address, stack_block_info_t **p_start_block_info,
                                              stack_block_info_t **p_end_block_info)
{
    // Get the start of the block info
    auto const start_block_info = (stack_block_info_t *)real_address;
    // Check for memory corruption
    if (allocator_check_guards((void *)(real_address + STACK_BLOCK_INFO_SIZE),
                               start_block_info->size - 2 * STACK_BLOCK_INFO_SIZE) == 0)
    {
        return CUTL_RESULT_CORRUPTED_POINTER;
    }

    if (start_block_info->state != MEMORY_BLOCK_USED)
    {
        if (start_block_info->state == MEMORY_BLOCK_FREE)
        {
            return CUTL_RESULT_DOUBLE_DEALLOCATION;
        }
        return CUTL_RESULT_CORRUPTED_POINTER;
    }
    auto const end_block_info = (stack_block_info_t *)(real_address + start_block_info->size - STACK_BLOCK_INFO_SIZE);
    // Check for corruption
    if (end_block_info->state != MEMORY_BLOCK_USED || end_block_info->size != start_block_info->size)
    {
        if (end_block_info->state == MEMORY_BLOCK_FREE)
        {
            return CUTL_RESULT_DOUBLE_DEALLOCATION;
        }
        return CUTL_RESULT_CORRUPTED_POINTER;
    }

    *p_start_block_info = start_block_info;
    *p_end_block_info = end_block_info;

    return CUTL_SUCCESS;
}

cutl_result_t cutl_allocator_stack_deallocate(cutl_allocator_stack_t *const this, void *const memory)
{
    auto real_address = validate_and_adjust_memory_address(this, memory);
    if (real_address == ~(uintptr_t)0)
        return CUTL_RESULT_MISMATCHED_ALLOCATOR;

    stack_block_info_t *end_block_info;
    stack_block_info_t *start_block_info;
    auto const res = stack_memory_info_blocks(real_address, &start_block_info, &end_block_info);
    if (res != CUTL_SUCCESS)
        return res;

    // Set the state of the block in its info blocks as free
    start_block_info->state = MEMORY_BLOCK_FREE;
    end_block_info->state = MEMORY_BLOCK_FREE;

    // Check if this allocation was the last from this allocator, meaning it is the top of the stack.
    // If not, we are done!
    if (real_address + start_block_info->size != (uintptr_t)this->data + this->top)
        return CUTL_SUCCESS;

    // Keep on popping until we have to stop and return.
    for (;;)
    {
        this->top -= end_block_info->size;
        // Check if we are actually at the start of the stack
        if (this->top == 0)
        {
            // This is the top, we are done.
            CUTL_ASSERT(this->data == (unsigned char *)real_address,
                        "Stack allocator is not at the start of the stack!");
            return CUTL_SUCCESS;
        }

        // As this is the last allocation, we can start popping free blocks from the stack. The info
        // about the block in front of this one sits just before it, stepping over any padding.
        end_block_info = (stack_block_info_t *)((unsigned char *)start_block_info - STACK_BLOCK_INFO_SIZE);
        // Adjust the block address to the previous one
        real_address -= end_block_info->size;
        start_block_info = (stack_block_info_t *)real_address;

        // Is there any corruption?
        if (end_block_info->size != start_block_info->size || end_block_info->state != start_block_info->state)
        {
            // The bookkeeping of the two blocks no longer agrees, so neither can be trusted.
            return CUTL_RESULT_CORRUPTED_POINTER;
        }

        // Is the previous block still being used?
        if (end_block_info->state == MEMORY_BLOCK_USED)
        {
            // Yes, it is, we are done
            return CUTL_SUCCESS;
        }

        // Was the block written out of bounds, or is the state simply not one we know?
        if (end_block_info->state != MEMORY_BLOCK_FREE)
            return CUTL_RESULT_CORRUPTED_POINTER;
    }
}

cutl_result_t cutl_allocator_stack_reallocate(cutl_allocator_stack_t *const this, void *const memory, const size_t size,
                                              void **const p_memory)
{
    auto real_address = validate_and_adjust_memory_address(this, memory);
    if (real_address == ~(uintptr_t)0)
        return CUTL_RESULT_MISMATCHED_ALLOCATOR;

    stack_block_info_t *end_block_info;
    stack_block_info_t *start_block_info;
    auto res = stack_memory_info_blocks(real_address, &start_block_info, &end_block_info);
    if (res != CUTL_SUCCESS)
        return res;

    // Adjust the size
    auto const adjusted_size = allocator_block_size(size) + 2 * STACK_BLOCK_INFO_SIZE;
    // Compare the requested size against the current block's size
    if (adjusted_size <= start_block_info->size)
    {
        // If we are shrinking, we do not do anything
        *p_memory = memory;
        return CUTL_SUCCESS;
    }

    // We may be able to grow this block in place if it ends at the top of the stack, as the memory
    // above it has not been handed out to anybody yet. Both sides have to be offsets for this to
    // mean anything, as the top is counted from the start of the stack.
    auto const block_offset = (size_t)(real_address - (uintptr_t)this->data);
    if (this->top == block_offset + start_block_info->size)
    {
        // Yes, we can grow it! The block ends at the top of the stack, so the top moves up with it.
        this->top += adjusted_size - start_block_info->size;
        auto const new_end = real_address + adjusted_size;
        start_block_info->size = adjusted_size;
        // Adjust the location of the end block
        end_block_info = (stack_block_info_t *)(new_end - STACK_BLOCK_INFO_SIZE);
        // Set up the end block
        end_block_info->state = MEMORY_BLOCK_USED;
        end_block_info->size = adjusted_size;
        // Set up the guard bytes
        allocator_set_guards((void *)(real_address + STACK_BLOCK_INFO_SIZE),
                             end_block_info->size - 2 * STACK_BLOCK_INFO_SIZE);
        // Return the original block's address
        *p_memory = memory;
        return CUTL_SUCCESS;
    }

    // NOTE: technically, if the next block is free, we can try merging the two, then keep on going.
    // We cannot grow it, so instead we just allocate a new block at the end of the stack.
    // The allocate function adds its own info blocks and guards, so it gets the raw size.
    res = cutl_allocator_stack_allocate(this, size, p_memory);
    if (res != CUTL_SUCCESS)
        return res;
    auto const old_capacity = allocator_usable_size(start_block_info->size - 2 * STACK_BLOCK_INFO_SIZE);
    // Copy the contents of the old block to the new one
    memcpy(*p_memory, memory, old_capacity < size ? old_capacity : size);
    // Deallocate the current block
    res = cutl_allocator_stack_deallocate(this, memory);
    CUTL_ASSERT(res == CUTL_SUCCESS, "Could not deallocate the old memory block: (%s) - %s", cutl_result_to_string(res),
                cutl_result_message(res));
    (void)res;
    // The old block is given back at the end, so the stack does not grow by more than it has to.
    return CUTL_SUCCESS;
}

void cutl_allocator_stack_reset(cutl_allocator_stack_t *this)
{
    this->top = 0;
}

uintptr_t cutl_allocator_stack_get_top(const cutl_allocator_stack_t *this)
{
    return this->top;
}

void cutl_allocator_stack_restore_top(cutl_allocator_stack_t *this, const uintptr_t top)
{
    // Only lower the top, never increase!
    if (this->top > top)
    {
        this->top = top;
    }
}

static void *wrap_stack_allocate(void *state, const size_t size)
{
    auto const allocator = (cutl_allocator_stack_t *)state;
    if (size == 0)
        return nullptr;
    void *memory;
    auto const res = cutl_allocator_stack_allocate(allocator, size, &memory);
    if (res != CUTL_SUCCESS)
    {
        CUTL_ASSERT(res == CUTL_RESULT_OUT_OF_MEMORY, "Could not allocate memory: (%s) - %s",
                    cutl_result_to_string(res), cutl_result_message(res));
        return nullptr;
    }
    return memory;
}

static void wrap_stack_deallocate(void *state, void *memory)
{
    auto const allocator = (cutl_allocator_stack_t *)state;
    auto const res = cutl_allocator_stack_deallocate(allocator, memory);
    CUTL_ASSERT(res == CUTL_SUCCESS, "Could not deallocate memory: (%s) - %s", cutl_result_to_string(res),
                cutl_result_message(res));
}

static void *wrap_stack_reallocate(void *state, void *memory, const size_t size)
{
    auto const allocator = (cutl_allocator_stack_t *)state;
    if (memory == nullptr)
        return wrap_stack_allocate(state, size);
    auto const res = cutl_allocator_stack_reallocate(allocator, memory, size, &memory);
    if (res != CUTL_SUCCESS)
    {
        CUTL_ASSERT(res == CUTL_RESULT_OUT_OF_MEMORY, "Could not reallocate memory: (%s) - %s",
                    cutl_result_to_string(res), cutl_result_to_string(res));
        return nullptr;
    }
    return memory;
}

cutl_result_t cutl_allocator_stack_create(const size_t size, unsigned char CUTL_ARRAY_ARG(memory, const size),
                                          cutl_allocator_stack_t **p_allocator)
{
    // Check the memory is aligned
    if (!allocator_is_aligned(memory))
        return CUTL_RESULT_INSUFFICIENT_ALIGNMENT;
    auto const allocator = (cutl_allocator_stack_t *)memory;
    allocator->size = size - sizeof(*allocator);
    allocator->top = 0;
    allocator->base = (cutl_allocator_t){
        .state = allocator,
        .allocate = wrap_stack_allocate,
        .deallocate = wrap_stack_deallocate,
        .reallocate = wrap_stack_reallocate,
    };
    *p_allocator = allocator;
    return CUTL_SUCCESS;
}

const cutl_allocator_t *cutl_allocator_stack_get(cutl_allocator_stack_t *const this)
{
    return &this->base;
}
