#include "../../include/cutl/allocators/arena_allocator.h"
#include "allocator_internal.h"

#include <stdint.h>

enum : uint32_t
{
    ARENA_BLOCK_USED = 0xA8E4A0B1, // Block is currently handed out to the user
    ARENA_BLOCK_FREE = 0xA8E4A0B2, // Block sits on the free list of its chunk
};

/**
 * Header stored in front of every block the arena hands out. It is all the arena needs to tell its
 * own blocks apart from foreign pointers, to spot blocks that were already given back, and to know
 * how much memory a block occupies when it is recycled or moved.
 */
typedef struct arena_block_t
{
    size_t size; // Total size of the block, this header included
    uint32_t magic;
} arena_block_t;

/**
 * Link a free block uses to point at the next one. It is kept in the memory of the block itself,
 * so that the free list does not need any memory of its own, and it sits behind the header, which
 * keeps the header to the two fields the arena cannot do without.
 */
typedef struct arena_free_node_t
{
    struct arena_free_node_t *next;
} arena_free_node_t;

// Padded so that the memory handed to the user keeps the alignment the allocator promises.
enum
{
    // Padded so that the memory handed to the user keeps the alignment the allocator promises.
    ARENA_BLOCK_HEADER_SIZE = CUTL_ALLOCATOR_ROUND_UP(sizeof(arena_block_t)),
    // The smallest block the arena hands out, which is therefore also the smallest a free block can be.
    ARENA_MINIMUM_BLOCK_SIZE = ARENA_BLOCK_HEADER_SIZE + ALLOCATOR_ALIGNMENT,
};

static_assert(ARENA_BLOCK_HEADER_SIZE % ALLOCATOR_ALIGNMENT == 0,
              "The block header must keep the memory handed to the user aligned.");
static_assert(ARENA_MINIMUM_BLOCK_SIZE >= ARENA_BLOCK_HEADER_SIZE + sizeof(arena_free_node_t),
              "A free block must be able to hold the link describing it on the free list.");

/**
 * A chunk of memory the arena took from its parent allocator, carved up into blocks.
 *
 * The part of the chunk that was never handed out is tracked by the bump offset, while everything
 * that was given back is kept on the free list. The free list is ordered by address, which is what
 * allows adjacent blocks to be merged back together.
 */
typedef struct arena_chunk_t
{
    struct arena_chunk_t *next;                        // Next chunk owned by the arena
    size_t capacity;                                   // Usable size of the memory below
    size_t bump;                                       // Offset of the first byte that was never handed out
    arena_free_node_t *free_list;                      // Recycled blocks, ordered by address
    alignas(ALLOCATOR_ALIGNMENT) unsigned char data[]; // Memory backing the arena's blocks
} arena_chunk_t;

struct cutl_allocator_arena_t
{
    cutl_allocator_t base;          // Allocator interface
    const cutl_allocator_t *parent; // Allocator the memory is taken from
    size_t chunk_size;              // Minimum size of a chunk requested from the parent
    size_t largest_block;           // Largest block the arena has been asked for
    size_t used;                    // Memory handed out to blocks, headers included
    size_t capacity;                // Memory owned across all of the chunks
    arena_chunk_t *chunks;          // Chunks owned by the arena
};

/**
 * Get the block a free list link belongs to, which is the block the link is stored in.
 *
 * @param node Link stored in a free block.
 * @return Block the link belongs to.
 */
static arena_block_t *block_of(const arena_free_node_t *const node)
{
    return (arena_block_t *)((const unsigned char *)node - ARENA_BLOCK_HEADER_SIZE);
}

/**
 * Get the free list link stored in a free block.
 *
 * @param block Free block to get the link of.
 * @return Link stored in the block.
 */
static arena_free_node_t *node_of(arena_block_t *const block)
{
    return (arena_free_node_t *)((unsigned char *)block + ARENA_BLOCK_HEADER_SIZE);
}

/**
 * Put a block back onto the free list of its chunk, merging it with the free blocks it touches so
 * that the arena does not slowly run out of memory it is unable to hand out.
 *
 * @param chunk Chunk the block belongs to.
 * @param block Block to add to the free list.
 */
static void free_list_insert(arena_chunk_t *const chunk, arena_block_t *const block)
{
    // Find the node the block has to be linked behind.
    arena_free_node_t **link = &chunk->free_list;
    while (*link != nullptr && block_of(*link) < block)
        link = &(*link)->next;

    arena_block_t *const previous = *link != nullptr ? block_of(*link) : nullptr;
    arena_block_t *merged;
    if (previous != nullptr && (unsigned char *)previous + previous->size == (unsigned char *)block)
    {
        // The block directly in front of it is free and adjacent, so grow that one instead.
        merged = previous;
        merged->size += block->size;
    }
    else
    {
        merged = block;
        node_of(block)->next = *link;
        *link = node_of(block);
    }

    // Same deal for the free block directly behind it, if the two touch.
    arena_free_node_t *const behind = node_of(merged)->next;
    if (behind != nullptr && (unsigned char *)merged + merged->size == (unsigned char *)behind)
    {
        merged->size += block_of(behind)->size;
        node_of(merged)->next = behind->next;
    }
}

/**
 * Find the free block of a chunk which starts at the given address.
 *
 * @param chunk Chunk to search.
 * @param address Address the free block has to start at.
 * @return The free block, or `nullptr` if no free block starts there.
 */
static arena_block_t *free_list_find(arena_chunk_t *const chunk, const unsigned char *const address)
{
    // The free list is ordered by address, so nothing past the address can match.
    for (arena_free_node_t *node = chunk->free_list; node != nullptr; node = node->next)
    {
        arena_block_t *const block = block_of(node);
        if ((unsigned char *)block == address)
            return block;
        if ((unsigned char *)block > address)
            break;
    }
    return nullptr;
}

/**
 * Unlink a free block from the free list of its chunk.
 *
 * @param chunk Chunk the free block belongs to.
 * @param block Free block to unlink.
 */
static void free_list_remove(arena_chunk_t *const chunk, arena_block_t *const block)
{
    for (arena_free_node_t **link = &chunk->free_list; *link != nullptr; link = &(*link)->next)
    {
        if (block_of(*link) == block)
        {
            *link = (*link)->next;
            return;
        }
    }
    CUTL_ASSERT(false, "Block %p is not on the free list of its chunk.", (void *)block);
}

/**
 * Hand out a block of the given total size from a chunk, preferring recycled memory over the part of
 * the chunk that was never handed out.
 *
 * @param chunk Chunk to carve the block out of.
 * @param total Total size of the block, header included.
 * @return The block, or `nullptr` if the chunk cannot satisfy the request.
 */
static arena_block_t *block_from_chunk(arena_chunk_t *const chunk, const size_t total)
{
    // Recycled memory comes first, so that blocks which were given back individually are put to use
    // before the arena reaches for the memory it has not touched yet.
    for (arena_free_node_t *node = chunk->free_list; node != nullptr; node = node->next)
    {
        arena_block_t *const block = block_of(node);
        if (block->size < total)
            continue;

        auto const available = block->size;
        free_list_remove(chunk, block);
        // Whatever is left of the recycled block goes back on the free list instead of being wasted.
        if (available - total >= ARENA_MINIMUM_BLOCK_SIZE)
        {
            arena_block_t *const remainder = (arena_block_t *)((unsigned char *)block + total);
            remainder->size = available - total;
            remainder->magic = ARENA_BLOCK_FREE;
            free_list_insert(chunk, remainder);
        }
        // The block is accounted for by the size it is handed out with, not by the larger block it
        // was carved out of, so that giving it back takes away exactly as much as it added.
        block->size = total;
        return block;
    }

    // Otherwise take the block off the end of the memory the chunk never handed out.
    if (chunk->capacity - chunk->bump < total)
        return nullptr;

    arena_block_t *const block = (arena_block_t *)(chunk->data + chunk->bump);
    block->size = total;
    chunk->bump += total;
    return block;
}

/**
 * Get the block and chunk a pointer handed out by the arena belongs to.
 *
 * @param this Arena the pointer is checked against.
 * @param memory Pointer to check.
 * @param p_chunk Address which receives the chunk the block belongs to.
 * @param p_block Address which receives the block.
 * @return CUTL_SUCCESS if successful, otherwise an error code indicating the reason for failure.
 */
static cutl_result_t block_from_pointer(const cutl_allocator_arena_t *const this, void *const memory,
                                        arena_chunk_t **const p_chunk, arena_block_t **const p_block)
{
    const unsigned char *const address = (const unsigned char *)memory;

    // The pointer has to fall into one of the chunks the arena owns.
    arena_chunk_t *chunk = nullptr;
    for (arena_chunk_t *it = this->chunks; it != nullptr; it = it->next)
    {
        if (address >= it->data && address < it->data + it->capacity)
        {
            chunk = it;
            break;
        }
    }
    if (chunk == nullptr)
        return CUTL_RESULT_MISMATCHED_ALLOCATOR;

    // User memory always starts behind the block's header.
    if ((size_t)(address - chunk->data) < ARENA_BLOCK_HEADER_SIZE)
        return CUTL_RESULT_MISMATCHED_ALLOCATOR;

    arena_block_t *const block = (arena_block_t *)(address - ARENA_BLOCK_HEADER_SIZE);
    if (block->magic == ARENA_BLOCK_FREE)
        return CUTL_RESULT_DOUBLE_DEALLOCATION;
    if (block->magic != ARENA_BLOCK_USED)
        return CUTL_RESULT_CORRUPTED_POINTER;

    *p_chunk = chunk;
    *p_block = block;
    return CUTL_SUCCESS;
}

/**
 * Ask the parent allocator for another chunk to carve blocks out of.
 *
 * @param this Arena to grow.
 * @param total Total size of the block the chunk has to hold.
 * @return CUTL_SUCCESS if successful, otherwise an error code indicating the reason for failure.
 */
static cutl_result_t arena_grow(cutl_allocator_arena_t *const this, const size_t total)
{
    // The chunk has to fit the block we need right now, and the largest block the arena was ever
    // asked for, so that such a block does not need a chunk of its own every time.
    auto const needed = total > this->largest_block ? total : this->largest_block;
    auto const capacity = allocator_round_up(needed > this->chunk_size ? needed : this->chunk_size);

    void *const memory = cutl_alloc(this->parent, sizeof(arena_chunk_t) + capacity);
    if (memory == nullptr)
        return CUTL_RESULT_OUT_OF_MEMORY;
    CUTL_ASSERT(allocator_is_aligned(memory), "The parent allocator handed out misaligned memory.");

    arena_chunk_t *const chunk = (arena_chunk_t *)memory;
    chunk->capacity = capacity;
    chunk->bump = 0;
    chunk->free_list = nullptr;
    chunk->next = this->chunks;
    this->chunks = chunk;
    this->capacity += capacity;
    if (total > this->largest_block)
        this->largest_block = total;

    return CUTL_SUCCESS;
}

/**
 * Prepare a block the arena is about to hand out.
 *
 * @param this Arena handing out the block.
 * @param block Block to hand out.
 * @param total Total size of the block, header included.
 * @param p_memory Address which receives the pointer to the block's memory.
 */
static void arena_hand_out(cutl_allocator_arena_t *const this, arena_block_t *const block, const size_t total,
                           void **const p_memory)
{
    block->magic = ARENA_BLOCK_USED;
    this->used += total;
    *p_memory = (unsigned char *)block + ARENA_BLOCK_HEADER_SIZE;
}

cutl_result_t cutl_allocator_arena_allocate(cutl_allocator_arena_t *const this, const size_t size,
                                            void **const p_memory)
{
    // There is no sensible address to hand out for a block without any memory in it.
    if (size == 0)
        return CUTL_RESULT_OUT_OF_MEMORY;

    auto const total = ARENA_BLOCK_HEADER_SIZE + allocator_round_up(size);
    // Reject requests whose bookkeeping would wrap around.
    if (total < size)
        return CUTL_RESULT_OUT_OF_MEMORY;

    // Try to satisfy the request from the memory the arena already owns.
    for (arena_chunk_t *chunk = this->chunks; chunk != nullptr; chunk = chunk->next)
    {
        arena_block_t *const block = block_from_chunk(chunk, total);
        if (block != nullptr)
        {
            arena_hand_out(this, block, total, p_memory);
            return CUTL_SUCCESS;
        }
    }

    // None of the chunks can hold it, so the parent allocator has to provide more memory.
    auto const res = arena_grow(this, total);
    if (res != CUTL_SUCCESS)
        return res;

    arena_block_t *const block = block_from_chunk(this->chunks, total);
    CUTL_ASSERT(block != nullptr, "A fresh chunk of %zu bytes could not hold a block of %zu bytes.",
                this->chunks->capacity, total);
    arena_hand_out(this, block, total, p_memory);
    return CUTL_SUCCESS;
}

cutl_result_t cutl_allocator_arena_deallocate(cutl_allocator_arena_t *const this, void *const memory)
{
    if (memory == nullptr)
        return CUTL_SUCCESS;

    arena_chunk_t *chunk;
    arena_block_t *block;
    auto const res = block_from_pointer(this, memory, &chunk, &block);
    if (res != CUTL_SUCCESS)
        return res;

    // Mark the block as free, so that the chunk can hand the memory out again later on.
    block->magic = ARENA_BLOCK_FREE;
    free_list_insert(chunk, block);
    this->used -= block->size;

    return CUTL_SUCCESS;
}

cutl_result_t cutl_allocator_arena_reallocate(cutl_allocator_arena_t *const this, void *const memory, const size_t size,
                                              void **const p_memory)
{
    // A block without any memory in it is the same as no block at all.
    if (size == 0)
    {
        auto const res = cutl_allocator_arena_deallocate(this, memory);
        *p_memory = nullptr;
        return res;
    }

    arena_chunk_t *chunk;
    arena_block_t *block;
    auto const res = block_from_pointer(this, memory, &chunk, &block);
    if (res != CUTL_SUCCESS)
        return res;

    auto const total = ARENA_BLOCK_HEADER_SIZE + allocator_round_up(size);
    if (total < size)
        return CUTL_RESULT_OUT_OF_MEMORY;

    // There is no reason to move the block if it is already large enough.
    if (total <= block->size)
    {
        *p_memory = memory;
        return CUTL_SUCCESS;
    }

    // Grow in place if the recycled block right behind this one makes up for the difference.
    arena_block_t *const behind = free_list_find(chunk, (unsigned char *)block + block->size);
    if (behind != nullptr && block->size + behind->size >= total)
    {
        auto const grown = block->size + behind->size;
        free_list_remove(chunk, behind);
        this->used += grown - block->size;
        block->size = grown;
        *p_memory = memory;
        return CUTL_SUCCESS;
    }

    // The block cannot grow where it is, so move it. The old block is in use, so the new one can
    // never overlap it, which is what makes copying the contents over safe.
    void *new_memory;
    auto const move_res = cutl_allocator_arena_allocate(this, size, &new_memory);
    if (move_res != CUTL_SUCCESS)
        return move_res;

    auto const copied = block->size < total ? block->size : total;
    memcpy(new_memory, memory, copied - ARENA_BLOCK_HEADER_SIZE);

    auto const free_res = cutl_allocator_arena_deallocate(this, memory);
    CUTL_ASSERT(free_res == CUTL_SUCCESS, "Could not deallocate the old block: (%s) - %s",
                cutl_result_to_string(free_res), cutl_result_message(free_res));

    *p_memory = new_memory;
    return CUTL_SUCCESS;
}

void cutl_allocator_arena_clear(cutl_allocator_arena_t *this)
{
    // Every block is invalidated at once, so the chunks only have to forget how far they got.
    for (arena_chunk_t *chunk = this->chunks; chunk != nullptr; chunk = chunk->next)
    {
        chunk->bump = 0;
        chunk->free_list = nullptr;
    }
    this->used = 0;
}

void cutl_allocator_arena_release(cutl_allocator_arena_t *this)
{
    // Hand every chunk back to the allocator the arena took its memory from.
    arena_chunk_t *chunk = this->chunks;
    while (chunk != nullptr)
    {
        arena_chunk_t *const next = chunk->next;
        this->parent->deallocate(this->parent->state, chunk);
        chunk = next;
    }
    this->chunks = nullptr;
    this->used = 0;
    this->capacity = 0;
    this->largest_block = 0;
}

void cutl_allocator_arena_destroy(cutl_allocator_arena_t *this)
{
    if (this == nullptr)
        return;

    const cutl_allocator_t *const parent = this->parent;
    cutl_allocator_arena_release(this);
    parent->deallocate(parent->state, this);
}

size_t cutl_allocator_arena_get_used(const cutl_allocator_arena_t *this)
{
    return this->used;
}

size_t cutl_allocator_arena_get_capacity(const cutl_allocator_arena_t *this)
{
    return this->capacity;
}

static void *wrap_arena_allocate(void *const state, const size_t size)
{
    cutl_allocator_arena_t *const arena = (cutl_allocator_arena_t *)state;
    if (size == 0)
        return nullptr;
    void *memory;
    auto const res = cutl_allocator_arena_allocate(arena, size, &memory);
    if (res != CUTL_SUCCESS)
    {
        CUTL_ASSERT(res == CUTL_RESULT_OUT_OF_MEMORY, "Could not allocate memory: (%s) - %s",
                    cutl_result_to_string(res), cutl_result_message(res));
        return nullptr;
    }
    return memory;
}

static void wrap_arena_deallocate(void *const state, void *const memory)
{
    cutl_allocator_arena_t *const arena = (cutl_allocator_arena_t *)state;
    auto const res = cutl_allocator_arena_deallocate(arena, memory);
    CUTL_ASSERT(res == CUTL_SUCCESS, "Could not deallocate memory: (%s) - %s", cutl_result_to_string(res),
                cutl_result_message(res));
}

static void *wrap_arena_reallocate(void *const state, void *const memory, const size_t size)
{
    cutl_allocator_arena_t *const arena = (cutl_allocator_arena_t *)state;
    if (memory == nullptr)
        return wrap_arena_allocate(state, size);
    void *new_memory;
    auto const res = cutl_allocator_arena_reallocate(arena, memory, size, &new_memory);
    if (res != CUTL_SUCCESS)
    {
        CUTL_ASSERT(res == CUTL_RESULT_OUT_OF_MEMORY, "Could not reallocate memory: (%s) - %s",
                    cutl_result_to_string(res), cutl_result_message(res));
        return nullptr;
    }
    return new_memory;
}

cutl_result_t cutl_allocator_arena_create(const cutl_allocator_t *const parent, const size_t chunk_size,
                                          cutl_allocator_arena_t **const p_arena)
{
    // Without a chunk size the arena would have no idea how much memory to ask for at a time.
    if (chunk_size == 0)
        return CUTL_RESULT_INSUFFICIENT_BUFFER;

    // Fall back to the default allocator if the caller did not name one. It is resolved once here, so
    // that the arena keeps handing its memory back where it got it.
    const cutl_allocator_t *const source = parent != nullptr ? parent : cutl_allocator_get_default();

    // The arena keeps track of itself in memory taken from the same allocator, so that a single
    // destroy hands everything back.
    cutl_allocator_arena_t *const arena = (cutl_allocator_arena_t *)cutl_alloc(source, sizeof(cutl_allocator_arena_t));
    if (arena == nullptr)
        return CUTL_RESULT_OUT_OF_MEMORY;

    arena->parent = source;
    arena->chunk_size = chunk_size;
    arena->largest_block = 0;
    arena->used = 0;
    arena->capacity = 0;
    arena->chunks = nullptr;
    arena->base = (cutl_allocator_t){
        .state = arena,
        .allocate = wrap_arena_allocate,
        .deallocate = wrap_arena_deallocate,
        .reallocate = wrap_arena_reallocate,
    };

    *p_arena = arena;
    return CUTL_SUCCESS;
}

const cutl_allocator_t *cutl_allocator_arena_get(cutl_allocator_arena_t *this)
{
    return &this->base;
}
