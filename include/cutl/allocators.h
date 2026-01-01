#pragma once
#include "common_defs.h"
#include <stddef.h>

/**
 * Interface for all other allocator functions. It contains the callback functions to call in order to make
 * allocations, along with a state pointer that is passed to all of these.
 */
typedef struct
{
    void *state;
    void *(*allocate)(void *state, size_t size);
    void (*deallocate)(void *state, void *ptr);
    void *(*reallocate)(void *state, void *ptr, size_t new_size);
} cutl_allocator_t;

/**
 * Standard C library allocator wrapped as cutl_allocator_t
 */
extern const cutl_allocator_t CUTL_STD_ALLOCATOR;

/**
 * Sets the allocator to use as the global default, in case no thread-local default is set.
 *
 * @note This copies over the `cutl_allocator_t` interface. As such, any state it references
 * must remain valid for as long as the global allocator is in use.
 *
 * @param allocator Allocator to use as default for global allocations.
 */
void cutl_allocator_set_global(const cutl_allocator_t *allocator);

/**
 * Sets the allocator to use as the thread-local default.
 *
 * @note This copies over the `cutl_allocator_t` interface. As such, any state it references
 * must remain valid for as long as the global allocator is in use.
 *
 * @param allocator Allocator to use as default for thread-local allocations.
 */
void cutl_allocator_set_thread(const cutl_allocator_t *allocator);

/**
 * Retrieve the default allocator for this thread.
 *
 * @return Default thread-local (if set) or global allocator.
 */
const cutl_allocator_t *cutl_allocator_get_default(void);

/**
 * Create a new memory allocation using the specified allocator.
 *
 * @param allocator Allocator to use for allocation.
 * @param size Size of allocation to make. If zero, nullptr is returned.
 * @return Pointer to newly allocated memory. If failed, nullptr is returned.
 */
void *cutl_alloc(const cutl_allocator_t *allocator, size_t size);

/**
 * Reallocate previously allocated memory to a block of a new size.
 *
 * @param allocator Allocator from which the allocation was originally created.
 * @param ptr Pointer to the allocation from the allocator. If null, `cutl_alloc` is called.
 * @param new_size New size desired size for the allocation. If zero, nullptr is returned, but ptr is still released.
 * @return Pointer to the newly allocated memory. If failed, nullptr is returned.
 */
void *cutl_realloc(const cutl_allocator_t *allocator, void *ptr, size_t new_size);

/**
 * Release the allocation back to the allocator.
 *
 * @param allocator Allocator from which the allocation was made.
 * @param ptr Pointer to the allocation received from the allocator.
 */
void cutl_dealloc(const cutl_allocator_t *allocator, void *ptr);

/**
 * Allocate a block using a default allocator.
 *
 * @param size Size of allocation to make. If zero, nullptr is returned.
 * @return Pointer to newly allocated memory. If failed, nullptr is returned.
 */
void *cutl_alloc_default(size_t size);

/**
 * Reallocate previously allocated memory using the default allocator.
 *
 * @param ptr Pointer to the allocation from the allocator. If null, `cutl_alloc` is called.
 * @param new_size New size desired size for the allocation. If zero, nullptr is returned, but ptr is still released.
 * @return Pointer to the newly allocated memory. If failed, nullptr is returned.
 */
void *cutl_realloc_default(void *ptr, size_t new_size);

/**
 * Release the allocation back to the default allocator.
 *
 * @param ptr Pointer to the allocation received from the allocator.
 */
void cutl_dealloc_default(void *ptr);
