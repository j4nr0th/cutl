#pragma once
#include "../error.h"
#include "allocator_internal.h"
#include "allocators.h"

typedef struct
{
    size_t offset;
    size_t size;
    memory_block_state_t state;
} memory_block_info_t;

// Allocator of fixed size
typedef struct
{
    cutl_allocator_t base;
    size_t total_size;      // Total size of the block
    size_t block_count;     // Total allocation count
    unsigned char memory[]; // Memory which includes allocations, as well as the block array at the end
} cutl_allocator_fs_t;

/** Create a fixed-size new allocator within the provided array.
 *
 * @param size Size of the memory within which to create the allocator.
 * @param memory Memory buffer within which to create the allocator. The allocator and all of its allocations
 * remain valid as long as this memory is valid.
 * @param p_allocator Pointer to and address which will receive the pointer to the allocator.
 *
 * @return CUTL_SUCCESS on success, otherwise an error code.
 */
cutl_result_t cutl_allocator_fs_create(size_t size, unsigned char CUTL_ARRAY_ARG(memory, size),
                                       cutl_allocator_fs_t **p_allocator);

/** Get the allocator interface from the fixed-size allocator.
 *
 * @param this Pointer to the fixed-size allocator.
 *
 * @return Allocator interface to the fixed-size allocator.
 */
const cutl_allocator_t *cutl_allocator_fs_get(const cutl_allocator_fs_t *this);

/**
 * Allocate a memory block of the specified size using the given fixed-size allocator.
 *
 * @param this Pointer to the fixed-size allocator from which memory should be allocated.
 * @param size Size of the memory block to allocate, in bytes.
 * @param p_memory Pointer to a variable that will receive the address of the allocated memory block
 * if the allocation succeeds.
 *
 * @return CUTL_SUCCESS if the memory allocation is successful. Otherwise, an appropriate error code
 * indicating the failure reason.
 */
cutl_result_t cutl_allocator_fs_allocate(cutl_allocator_fs_t *this, size_t size, void **p_memory);

/**
 * Deallocate a specific memory block previously allocated by the fixed-size allocator.
 *
 * @param this Pointer to the fixed-size allocator that owns the memory.
 * @param memory Pointer to the memory block to be deallocated.
 * Must be a valid block obtained from the same allocator.
 *
 * @return CUTL_SUCCESS if the memory block was successfully deallocated. Otherwise, an appropriate error code
 * indicating the failure reason.
 */
cutl_result_t cutl_allocator_fs_deallocate(cutl_allocator_fs_t *this, void *memory);

/**
 * Reallocate memory using the fixed-size allocator. Adjusts the size of an existing memory
 * allocation, potentially allocating a new memory block and moving data to suit the required size.
 *
 * @param this Pointer to the fixed-size allocator instance managing the memory pool.
 * @param size New size to allocate for the memory block.
 * @param old_ptr Pointer to the previously allocated memory block to be resized.
 * @param p_memory Pointer to an address which will receive the pointer to the reallocated memory.
 *
 * @return CUTL_SUCCESS if the memory block was successfully reallocated. Otherwise, an appropriate error code
 * indicating the failure reason.
 */
cutl_result_t cutl_allocator_fs_reallocate(cutl_allocator_fs_t *this, size_t size, void *old_ptr, void **p_memory);

/**
 * Retrieve the actual size of a memory block allocated within the fixed-size allocator.
 *
 * @param this Pointer to the fixed-size allocator instance.
 * @param memory Pointer to the memory block for which the actual size is being queried.
 * @param p_size Pointer to a variable where the size of the memory block will be stored.
 *
 * @return CUTL_SUCCESS if the size is successfully retrieved.
 *         CUTL_RESULT_MISMATCHED_ALLOCATOR if the memory block does not belong to the allocator.
 *         CUTL_RESULT_CORRUPTED_POINTER if the memory block appears invalid or corrupted.
 */
cutl_result_t cutl_allocator_fs_real_block_size(const cutl_allocator_fs_t *this, void *memory, size_t *p_size);
