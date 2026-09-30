#pragma once

#include "../allocators.h"
#include "../error.h"

#include <stddef.h>

/**
 * Allocator which hands out memory carved out of large chunks that it requests from another
 * allocator. It is well suited for workflows in which a lot of scratch buffers are allocated inside
 * a function and released right after it.
 *
 * Since the arena takes its memory from another allocator, the memory it deals with is properly owned
 * and does not have to be provided by the caller. Releasing the memory of many allocations at once
 * is what the arena is good at, which is done either by
 * `cutl_allocator_arena_clear`, which releases every allocation while keeping the memory around for
 * reuse, or by `cutl_allocator_arena_release`, which returns all of the memory to the allocator the
 * arena was created with.
 *
 * Individual allocations can also be given back with `cutl_allocator_arena_deallocate`, in which case
 * the memory is recycled for subsequent allocations. Doing so is not required in order to reuse the
 * memory, and the arena never has to walk a list of its individual allocations in order to release
 * them all at once.
 */
typedef struct cutl_allocator_arena_t cutl_allocator_arena_t;

/**
 * Create an arena allocator, which takes its memory from another allocator.
 *
 * The arena requests its memory in chunks of at least `chunk_size` bytes. Chunks grow to fit the
 * largest block the arena has been asked for, so that repeatedly allocating such a block does not
 * fragment the arena.
 *
 * @param parent Allocator the arena takes its memory from. If `nullptr`, the default allocator is
 *               used, resolved at the time of this call.
 * @param chunk_size Minimum size of the chunks the arena requests from `parent`. Must be greater
 *                   than zero.
 * @param p_arena Address which receives the created arena.
 * @return CUTL_SUCCESS if successful, otherwise an error code indicating the reason for failure.
 */
cutl_result_t cutl_allocator_arena_create(const cutl_allocator_t *parent, size_t chunk_size,
                                          cutl_allocator_arena_t **p_arena);

/**
 * Get the allocator interface for the arena allocator.
 *
 * @param this Arena to get the interface for.
 * @return Pointer to the allocator interface.
 */
const cutl_allocator_t *cutl_allocator_arena_get(cutl_allocator_arena_t *this);

/**
 * Allocate a memory block of the requested size from the arena.
 *
 * @param this Arena to allocate the block from.
 * @param size Size of the block to allocate. Must be greater than zero.
 * @param p_memory Address which receives the pointer to the allocated block.
 * @return CUTL_SUCCESS if successful, otherwise an error code indicating the reason for failure.
 */
cutl_result_t cutl_allocator_arena_allocate(cutl_allocator_arena_t *this, size_t size, void **p_memory);

/**
 * Return a block which was allocated from the arena. The memory is recycled for later allocations,
 * so that it does not have to be requested from the parent allocator again.
 *
 * @param this Arena the block was allocated from.
 * @param memory Memory which was allocated from the arena.
 * @return CUTL_SUCCESS if successful, otherwise an error code indicating the reason for failure.
 */
cutl_result_t cutl_allocator_arena_deallocate(cutl_allocator_arena_t *this, void *memory);

/**
 * Reallocate a block of memory within the arena, keeping its contents.
 *
 * The block is only moved if it cannot grow in place. A request for a size of zero releases the
 * block and sets the resulting memory to `nullptr`.
 *
 * @param this Arena the block was allocated from.
 * @param memory Memory which was allocated from the arena.
 * @param size New size of the desired allocation.
 * @param p_memory Address which receives the newly reallocated memory.
 * @return CUTL_SUCCESS if successful, otherwise an error code indicating the reason for failure.
 */
cutl_result_t cutl_allocator_arena_reallocate(cutl_allocator_arena_t *this, void *memory, size_t size, void **p_memory);

/**
 * Release every allocation made from the arena at once, marking all of its memory as free and
 * available for use again.
 *
 * The memory itself is kept, so that the arena does not have to request it from its parent allocator
 * again. This is the operation to use for scratch buffers that are released once the function they
 * belong to is done with them. Any block that was not given back individually is invalidated.
 *
 * @param this Arena to clear.
 */
void cutl_allocator_arena_clear(cutl_allocator_arena_t *this);

/**
 * Return all of the memory owned by the arena back to the allocator it was created with, leaving the
 * arena empty and ready to be used again.
 *
 * Unlike `cutl_allocator_arena_clear`, this hands the memory over to the parent allocator, so the
 * next allocation has to request memory from it again. Any block that was allocated from the arena
 * is invalidated.
 *
 * @param this Arena to release.
 */
void cutl_allocator_arena_release(cutl_allocator_arena_t *this);

/**
 * Return all of the memory owned by the arena back to its parent allocator and destroy the arena
 * itself. The arena must not be used afterwards.
 *
 * @param this Arena to destroy.
 */
void cutl_allocator_arena_destroy(cutl_allocator_arena_t *this);

/**
 * Get the amount of memory the arena currently hands out to its allocations, including the bookkeeping
 * it keeps for each of them. Blocks which were given back are not counted.
 *
 * @param this Arena to query.
 * @return Amount of memory in use by the arena, in bytes.
 */
size_t cutl_allocator_arena_get_used(const cutl_allocator_arena_t *this);

/**
 * Get the total amount of memory the arena owns across all of its chunks.
 *
 * @param this Arena to query.
 * @return Amount of memory owned by the arena, in bytes.
 */
size_t cutl_allocator_arena_get_capacity(const cutl_allocator_arena_t *this);
