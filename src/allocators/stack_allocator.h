#pragma once
#include "../error.h"
#include "allocators.h"

#include <stddef.h>
#include <stdint.h>

/**
 * Allocator well suited for FIFO allocations. Allocations are made and freed from the top of the stack.
 */
typedef struct cutl_allocator_stack_t cutl_allocator_stack_t;


/**
 * Create a new stack allocator from the block of memory.
 *
 * @param size Size of memory from which the allocator is created.
 * @param memory Memory from which the allocator is created.
 * @param p_allocator Pointer which receives the created allocator.
 * @return CUTL_SUCCESS if successful, otherwise an error code indicating the reason for failure.
 */
cutl_result_t cutl_allocator_stack_create(size_t size, unsigned char CUTL_ARRAY_ARG(memory, size),
                                          cutl_allocator_stack_t **p_allocator);

/**
 * Get the `cutl_allocator_t` interface from the stack allocator.
 *
 * @param this Stack allocator from which the allocator interface is taken from.
 * @return Allocator interface.
 */
const cutl_allocator_t *cutl_allocator_stack_get(cutl_allocator_stack_t *this);

/**
 * Allocate a memory block of the requested size from the stack.
 *
 * @param this Stack from which the memory is allocated from.
 * @param size Size of the block to allocate.
 * @param p_memory Address which receives the address of the allocated block.
 * @return CUTL_SUCCESS if successful, otherwise an error code indicating the reason for failure.
 */
cutl_result_t cutl_allocator_stack_allocate(cutl_allocator_stack_t *this, size_t size, void **p_memory);

/**
 * Deallocate the memory block from the stack allocator.
 *
 * @param this Pointer to the stack allocator from which the block was previously allocated.
 * @param memory Pointer to the memory block to be deallocated.
 * @return CUTL_SUCCESS if successful, otherwise an error code indicating the reason for failure.
 */
cutl_result_t cutl_allocator_stack_deallocate(cutl_allocator_stack_t *this, void *memory);

/**
 * Reallocate a block of memory within the stack allocator.
 *
 * @param this A pointer to the stack allocator.
 * @param memory The currently allocated block to be reallocated.
 * @param size The new size for the block of memory.
 * @param p_memory A pointer to a location that receives the reallocated memory block.
 * @return CUTL_SUCCESS if successful, otherwise an error code indicating the reason for failure.
 */
cutl_result_t cutl_allocator_stack_reallocate(cutl_allocator_stack_t *this, void *memory, size_t size, void **p_memory);

/**
 * Resets the allocator by marking all its memory as free and available for use.
 *
 * @param this Allocator to reset.
 */
void cutl_allocator_stack_reset(cutl_allocator_stack_t *this);

/**
 * Get the top of the stack allocator. This can be used by then calling `cutl_allocator_stack_restore_top`
 * to restore the state of the stack.
 *
 * @param this Allocator to get the top from.
 * @return Current position of the stack top.
 */
uintptr_t cutl_allocator_stack_get_top(const cutl_allocator_stack_t *this);

/**
 * Reset the top of the stack allocator to a value previously obtained by a call to
 * `cutl_allocator_stack_get_top`. This causes all allocations made since that call to
 * be released.
 *
 * @param this Allocator to restore the top of the stack to.
 * @param top Top of the stack position.
 */
void cutl_allocator_stack_restore_top(cutl_allocator_stack_t *this, uintptr_t top);
