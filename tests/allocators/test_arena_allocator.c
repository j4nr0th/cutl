#include "allocator_common_test.h"
#include <cutl/allocators/arena_allocator.h>
#include <cutl/allocators/stack_allocator.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/**
 * Counts how often the arena reaches for its parent allocator, so that the tests can tell the
 * difference between the arena recycling memory it already owns and the arena asking for more.
 */
typedef struct
{
    unsigned allocations; // Number of times memory was requested
    unsigned releases;    // Number of times memory was given back
    size_t live;          // Amount of memory currently held from the parent
} counting_state_t;

static counting_state_t COUNTING = {0, 0, 0};

enum : size_t
{
    // Room kept in front of every block for its size. A whole alignment unit, so that the memory
    // handed out stays aligned and the original pointer can be recovered exactly on the way back.
    COUNTING_PREFIX = TEST_ALLOCATOR_ALIGNMENT,
};

static void *counting_allocate(void *const state, const size_t size)
{
    counting_state_t *const counting = (counting_state_t *)state;
    // The arena expects memory handed to it to be aligned, which the standard allocator does not
    // promise once the library has been built for a stricter alignment than the usual one.
    auto const total =
        (size + 2 * COUNTING_PREFIX + TEST_ALLOCATOR_ALIGNMENT - 1) & ~(size_t)(TEST_ALLOCATOR_ALIGNMENT - 1);
    unsigned char *const raw = aligned_alloc(TEST_ALLOCATOR_ALIGNMENT, total);
    if (raw == nullptr)
        return nullptr;
    *(size_t *)raw = size;
    counting->allocations += 1;
    counting->live += size;
    return raw + COUNTING_PREFIX;
}

static void counting_deallocate(void *const state, void *const ptr)
{
    counting_state_t *const counting = (counting_state_t *)state;
    unsigned char *const raw = (unsigned char *)ptr - COUNTING_PREFIX;
    counting->live -= *(size_t *)raw;
    counting->releases += 1;
    free(raw);
}

static void *counting_reallocate(void *const state, void *const ptr, const size_t size)
{
    if (ptr == nullptr)
        return counting_allocate(state, size);
    if (size == 0)
    {
        counting_deallocate(state, ptr);
        return nullptr;
    }

    void *const memory = counting_allocate(state, size);
    if (memory == nullptr)
        return nullptr;
    auto const old_size = *(size_t *)((unsigned char *)ptr - COUNTING_PREFIX);
    memcpy(memory, ptr, old_size < size ? old_size : size);
    counting_deallocate(state, ptr);
    return memory;
}

static const cutl_allocator_t COUNTING_ALLOCATOR = {
    .state = &COUNTING,
    .allocate = counting_allocate,
    .deallocate = counting_deallocate,
    .reallocate = counting_reallocate,
};

enum : size_t
{
    CHUNK_SIZE = 64 * 1024,
};

/**
 * Check that blocks come back aligned the way the allocator promises, and that all of the requested
 * memory is actually there to be written to.
 */
static void test_alignment(cutl_allocator_arena_t *const arena)
{
    const auto interface = cutl_allocator_arena_get(arena);
    for (size_t size = 1; size <= 512; size *= 2)
    {
        void *const memory = cutl_alloc(interface, size);
        TEST_ASSERTION(memory != nullptr, "Failed to allocate %zu bytes.", size);
        TEST_ASSERTION(((uintptr_t)memory % alignof(max_align_t)) == 0,
                       "Block of %zu bytes was not aligned to max_align_t (address %p).", size, memory);
        // The whole block has to be usable, so writing to its very last byte must be safe.
        memset(memory, 0xAB, size);
        TEST_ASSERTION(((unsigned char *)memory)[size - 1] == 0xAB, "Last byte of the block was lost.");
        cutl_dealloc(interface, memory);
    }
}

/**
 * Check that a block which was given back is handed out again instead of being lost, and that the
 * arena does not ask its parent for more memory while it already owns enough.
 */
static void test_recycling(cutl_allocator_arena_t *const arena)
{
    cutl_result_t res;
    auto const interface = cutl_allocator_arena_get(arena);
    // Start from a clean arena, so that the amounts below do not depend on what ran before.
    cutl_allocator_arena_clear(arena);
    auto const capacity = cutl_allocator_arena_get_capacity(arena);
    TEST_ASSERTION(capacity >= CHUNK_SIZE, "Arena owns %zu bytes instead of at least %zu.", capacity,
                   (size_t)CHUNK_SIZE);

    auto const allocations = COUNTING.allocations;
    void *const first = cutl_alloc(interface, 1024);
    TEST_ASSERTION(first != nullptr, "Failed to allocate the first block.");
    void *const second = cutl_alloc(interface, 1024);
    TEST_ASSERTION(second != nullptr, "Failed to allocate the second block.");
    TEST_ASSERTION(first != second, "Two blocks were handed out at the same address.");

    // Hand both back and ask for the same amount again. The memory is recycled, so the arena must
    // not have grown while doing so.
    TEST_CUTL_RESULT(res, cutl_allocator_arena_deallocate(arena, first), CUTL_SUCCESS);
    TEST_CUTL_RESULT(res, cutl_allocator_arena_deallocate(arena, second), CUTL_SUCCESS);
    TEST_ASSERTION(cutl_allocator_arena_get_used(arena) == 0, "Arena still counts %zu bytes as used.",
                   cutl_allocator_arena_get_used(arena));

    TEST_ASSERTION(cutl_alloc(interface, 1024) != nullptr, "Failed to allocate from the recycled memory.");
    TEST_ASSERTION(cutl_alloc(interface, 1024) != nullptr, "Failed to allocate from the recycled memory.");
    TEST_ASSERTION(COUNTING.allocations == allocations,
                   "Arena asked its parent for memory (%u times) although it owned enough already.",
                   COUNTING.allocations - allocations);
    TEST_ASSERTION(cutl_allocator_arena_get_capacity(arena) == capacity, "Arena grew from %zu to %zu bytes.", capacity,
                   cutl_allocator_arena_get_capacity(arena));
}

/**
 * Check that clearing releases every allocation at once, and that the memory stays available for
 * reuse without the parent allocator being involved at all.
 */
static void test_clear(cutl_allocator_arena_t *const arena)
{
    auto const interface = cutl_allocator_arena_get(arena);
    // Hand out more than a single chunk holds, so that the arena is sure to own a few of them.
    for (unsigned i = 0; i < 64; ++i)
    {
        TEST_ASSERTION(cutl_alloc(interface, 2048) != nullptr, "Failed to allocate block %u.", i);
    }
    TEST_ASSERTION(cutl_allocator_arena_get_used(arena) > 0, "Arena reports nothing as used.");

    auto const capacity = cutl_allocator_arena_get_capacity(arena);
    auto const allocations = COUNTING.allocations;
    auto const releases = COUNTING.releases;

    cutl_allocator_arena_clear(arena);

    TEST_ASSERTION(cutl_allocator_arena_get_used(arena) == 0,
                   "Arena still counts %zu bytes as used after being "
                   "cleared.",
                   cutl_allocator_arena_get_used(arena));
    TEST_ASSERTION(cutl_allocator_arena_get_capacity(arena) == capacity, "Clearing gave away %zu bytes of memory.",
                   capacity - cutl_allocator_arena_get_capacity(arena));
    TEST_ASSERTION(COUNTING.releases == releases, "Clearing handed %u chunks back to the parent allocator.",
                   COUNTING.releases - releases);

    // All of the memory is available again, without asking the parent for a single byte of it.
    TEST_ASSERTION(cutl_alloc(interface, 2048) != nullptr, "Failed to allocate after clearing.");
    TEST_ASSERTION(COUNTING.allocations == allocations, "Allocating after a clear needed new memory from the "
                                                        "parent allocator.");
    TEST_ASSERTION(cutl_allocator_arena_get_capacity(arena) == capacity, "Arena had to grow after being cleared.");
}

/**
 * Check that releasing gives every chunk back to the parent allocator, and leaves behind an arena
 * which can be used right away.
 */
static void test_release(cutl_allocator_arena_t *const arena, const size_t baseline)
{
    auto const interface = cutl_allocator_arena_get(arena);
    // Make sure the arena owns more than a single chunk.
    for (unsigned i = 0; i < 128; ++i)
    {
        TEST_ASSERTION(cutl_alloc(interface, 1024) != nullptr, "Failed to allocate block %u.", i);
    }
    auto const capacity = cutl_allocator_arena_get_capacity(arena);
    auto const releases = COUNTING.releases;
    TEST_ASSERTION(capacity > 0, "Arena owns no memory to release.");

    cutl_allocator_arena_release(arena);

    TEST_ASSERTION(cutl_allocator_arena_get_capacity(arena) == 0, "Arena still reports owning %zu bytes.",
                   cutl_allocator_arena_get_capacity(arena));
    TEST_ASSERTION(cutl_allocator_arena_get_used(arena) == 0, "Arena still reports %zu bytes as used.",
                   cutl_allocator_arena_get_used(arena));
    TEST_ASSERTION(COUNTING.releases > releases, "Releasing did not hand any chunk back to the parent allocator.");
    // Every chunk the arena asked for went back, so the parent is left holding nothing but the small
    // bit the arena needs to keep track of itself.
    TEST_ASSERTION(COUNTING.live == baseline, "Parent still holds %zu bytes of the arena's chunks.",
                   COUNTING.live - baseline);

    // The arena is empty, but perfectly usable again.
    TEST_ASSERTION(cutl_alloc(interface, 4096) != nullptr, "Failed to allocate after releasing.");
    TEST_ASSERTION(cutl_allocator_arena_get_capacity(arena) > 0, "Arena did not take any memory again.");
}

/**
 * Check that reallocating keeps the contents of a block, both when the block stays where it is and
 * when it has to be moved, and that it grows into recycled memory instead of moving for that.
 */
static void test_reallocate(cutl_allocator_arena_t *const arena)
{
    cutl_result_t res;
    auto const interface = cutl_allocator_arena_get(arena);
    for (unsigned char fill = 0; fill < 16; ++fill)
    {
        void *const block = cutl_alloc(interface, 512);
        TEST_ASSERTION(block != nullptr, "Failed to allocate a block.");
        memset(block, fill, 512);

        // Shrinking keeps the block where it is, along with its contents.
        void *shrunk;
        TEST_CUTL_RESULT(res, cutl_allocator_arena_reallocate(arena, block, 128, &shrunk), CUTL_SUCCESS);
        TEST_ASSERTION(shrunk == block, "Shrinking a block moved it.");
        TEST_ASSERTION(((unsigned char *)shrunk)[0] == fill && ((unsigned char *)shrunk)[127] == fill,
                       "Shrinking a block lost its contents.");

        // Growing past what the block can hold has to move it, but keeps the contents.
        void *grown;
        TEST_CUTL_RESULT(res, cutl_allocator_arena_reallocate(arena, shrunk, 8192, &grown), CUTL_SUCCESS);
        TEST_ASSERTION(((unsigned char *)grown)[0] == fill && ((unsigned char *)grown)[127] == fill,
                       "Growing a block lost its contents.");
        TEST_CUTL_RESULT(res, cutl_allocator_arena_deallocate(arena, grown), CUTL_SUCCESS);
    }

    // Start from a clean arena, so that the two blocks below are carved out of the same chunk one
    // right after the other, which is what lets one grow into the other.
    cutl_allocator_arena_clear(arena);

    // A block with recycled memory right behind it grows in place rather than being moved.
    void *const block = cutl_alloc(interface, 256);
    TEST_ASSERTION(block != nullptr, "Failed to allocate a block.");
    memset(block, 0x5A, 256);
    void *const behind = cutl_alloc(interface, 256);
    TEST_ASSERTION(behind != nullptr, "Failed to allocate the block behind it.");
    TEST_CUTL_RESULT(res, cutl_allocator_arena_deallocate(arena, behind), CUTL_SUCCESS);

    void *grown;
    TEST_CUTL_RESULT(res, cutl_allocator_arena_reallocate(arena, block, 384, &grown), CUTL_SUCCESS);
    TEST_ASSERTION(grown == block, "A block with recycled memory behind it was moved instead of grown.");
    TEST_ASSERTION(((unsigned char *)grown)[0] == 0x5A && ((unsigned char *)grown)[255] == 0x5A,
                   "Growing a block in place lost its contents.");
    TEST_CUTL_RESULT(res, cutl_allocator_arena_deallocate(arena, grown), CUTL_SUCCESS);

    cutl_allocator_arena_clear(arena);
}

/**
 * Check the failures the arena reports, rather than handing out or accepting memory that is not its
 * own.
 */
static void test_error_reporting(cutl_allocator_arena_t *const arena)
{
    cutl_result_t res;
    auto const interface = cutl_allocator_arena_get(arena);

    // A block without any memory in it cannot be handed out.
    void *memory = nullptr;
    TEST_CUTL_RESULT(res, cutl_allocator_arena_allocate(arena, 0, &memory), CUTL_RESULT_OUT_OF_MEMORY);
    TEST_ASSERTION(memory == nullptr, "An empty block was handed out anyway.");

    // Memory the arena never allocated is not its to give back.
    int on_the_stack = 0;
    TEST_CUTL_RESULT(res, cutl_allocator_arena_deallocate(arena, &on_the_stack), CUTL_RESULT_MISMATCHED_ALLOCATOR);

    // A block can only be given back once.
    void *const block = cutl_alloc(interface, 128);
    TEST_ASSERTION(block != nullptr, "Failed to allocate a block.");
    TEST_CUTL_RESULT(res, cutl_allocator_arena_deallocate(arena, block), CUTL_SUCCESS);
    TEST_CUTL_RESULT(res, cutl_allocator_arena_deallocate(arena, block), CUTL_RESULT_DOUBLE_DEALLOCATION);

    // Memory the arena does not own cannot be reallocated either.
    void *other;
    TEST_CUTL_RESULT(res, cutl_allocator_arena_reallocate(arena, &on_the_stack, 128, &other),
                     CUTL_RESULT_MISMATCHED_ALLOCATOR);

    // Asking for more than the parent can possibly provide fails, rather than being faked.
    TEST_CUTL_RESULT(res, cutl_allocator_arena_allocate(arena, SIZE_MAX / 2, &memory), CUTL_RESULT_OUT_OF_MEMORY);

    // An arena without a chunk size has nothing to ask the parent allocator for.
    cutl_allocator_arena_t *useless;
    TEST_CUTL_RESULT(res, cutl_allocator_arena_create(&COUNTING_ALLOCATOR, 0, &useless),
                     CUTL_RESULT_INSUFFICIENT_BUFFER);
}

/**
 * Check that the arena hands its memory back to a parent which is not the standard allocator, which
 * is what makes it usable on top of the other allocators in this library.
 */
static void test_on_top_of_another_allocator(void)
{
    cutl_result_t res;
    enum
    {
        STACK_SIZE = 512 * 1024,
    };
    unsigned char *const memory = test_aligned_alloc(STACK_SIZE);
    TEST_ASSERTION(memory != nullptr, "Failed to allocate the stack's memory.");

    cutl_allocator_stack_t *stack;
    TEST_CUTL_RESULT(res, cutl_allocator_stack_create(STACK_SIZE, memory, &stack), CUTL_SUCCESS);

    cutl_allocator_arena_t *arena;
    TEST_CUTL_RESULT(res, cutl_allocator_arena_create(cutl_allocator_stack_get(stack), 64 * 1024, &arena),
                     CUTL_SUCCESS);
    auto const interface = cutl_allocator_arena_get(arena);

    for (unsigned i = 0; i < 256; ++i)
    {
        void *const block = cutl_alloc(interface, 1024);
        TEST_ASSERTION(block != nullptr, "Failed to allocate block %u from the arena.", i);
        memset(block, i, 1024);
    }
    TEST_ASSERTION(cutl_allocator_arena_get_capacity(arena) > 0, "Arena owns no memory.");

    // Giving all of the memory back to the stack must leave the stack usable.
    cutl_allocator_arena_destroy(arena);
    void *const after = cutl_alloc(cutl_allocator_stack_get(stack), 1024);
    TEST_ASSERTION(after != nullptr, "The stack allocator could not be used after the arena was destroyed.");
    cutl_dealloc(cutl_allocator_stack_get(stack), after);

    free(memory);
}

int main(int argc, char *CUTL_ARRAY_ARG(argv, static argc))
{
    (void)argc;
    (void)argv;
    cutl_result_t res;

    cutl_allocator_arena_t *arena;
    TEST_CUTL_RESULT(res, cutl_allocator_arena_create(&COUNTING_ALLOCATOR, CHUNK_SIZE, &arena), CUTL_SUCCESS);
    // What the parent is left with once the arena hands all of its memory back, which is only the
    // bookkeeping the arena needs to keep track of itself.
    auto const baseline = COUNTING.live;
    test_alignment(arena);
    test_recycling(arena);
    test_clear(arena);
    test_release(arena, baseline);
    test_reallocate(arena);
    test_error_reporting(arena);

    // Finally, hold the arena to the same standard as the other allocators: allocate, reallocate and
    // deallocate in a random order, checking that the contents survive all of it.
    test_allocator(cutl_allocator_arena_get(arena), 512, 32, 2049, 235723509, 10);

    // Destroying the arena must not leave a single byte behind with the parent allocator.
    cutl_allocator_arena_destroy(arena);
    TEST_ASSERTION(COUNTING.live == 0, "%zu bytes are still held by the parent allocator.", COUNTING.live);
    TEST_ASSERTION(COUNTING.allocations == COUNTING.releases,
                   "The arena took %u allocations from its parent, but only gave %zu back.", COUNTING.allocations,
                   COUNTING.releases);

    test_on_top_of_another_allocator();
    return 0;
}
