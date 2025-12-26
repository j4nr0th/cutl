#pragma once

#include "../error.h"
#include "allocators.h"
#include <stddef.h>

typedef struct
{
    cutl_allocator_t base;                       // Allocator interface
    unsigned block_size;                         // Size of individual blocks
    unsigned block_count;                        // Number of blocks in the allocator
    alignas(max_align_t) unsigned char memory[]; // Memory used to back the allocations and store the block states
} cutl_allocator_block_t;

/**
 * Create a block allocator, which only allocates blocks of fixed size.
 *
 * @param size Size of the memory where to create the allocator.
 * @param memory Memory in which to create the allocator.
 * @param block_size Size of blocks to allocate with this allocator.
 * @param p_allocator Pointer which receives the allocator.
 * @return CUTL_SUCCESS if successful, otherwise an error code indicating the reason for failure.
 */
cutl_result_t cutl_allocator_block_create(size_t size, unsigned char CUTL_ARRAY_ARG(memory, size), unsigned block_size,
                                          cutl_allocator_block_t **p_allocator);

/**
 * Get the allocator interface for the block allocator.
 *
 * @param this Block allocator to get the interface for.
 * @return Pointer to the allocator interface.
 */
const cutl_allocator_t *cutl_allocator_block_get(cutl_allocator_block_t *this);

/**
 * Get a new block from the allocator.
 *
 * @param this Allocator to allocate the block from.
 * @param p_memory Address, which receives the pointer to the allocated memory block.
 * @return CUTL_SUCCESS if successful, otherwise an error code indicating the reason for failure.
 */
cutl_result_t cutl_allocator_block_allocate(cutl_allocator_block_t *this, void **p_memory);

/**
 * Return a block which was allocated back to the allocator.
 *
 * @param this Allocator to return the block to.
 * @param memory Memory which was allocated from the allocator.
 * @return CUTL_SUCCESS if successful, otherwise an error code indicating the reason for failure.
 */
cutl_result_t cutl_allocator_block_deallocate(cutl_allocator_block_t *this, void *memory);

/**
 * Mimics a reallocation. This does just return the same address if the size is less than the block size,
 * otherwise returns an error.
 *
 * @param this Allocator from which to reallocate.
 * @param memory Pointer to memory allocated from the allocator.
 * @param size New size of the desired allocation.
 * @param p_memory Address which receives the newly reallocated memory.
 * @return CUTL_SUCCESS if successful, otherwise an error code indicating the reason for failure.
 */
cutl_result_t cutl_allocator_block_reallocate(cutl_allocator_block_t *this, void *memory, size_t size, void **p_memory);

/**
 * Find the real usable size of blocks, which is typically larger to satisfy alignment requirements.
 *
 * @param this Allocator to get the block size from.
 * @return Real usable size of blocks.
 */
unsigned cutl_allocator_block_get_block_size(const cutl_allocator_block_t *this);
