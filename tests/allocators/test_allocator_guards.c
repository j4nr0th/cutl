#include "../common/common.h"
#include <cutl/allocators/arena_allocator.h>
#include <cutl/allocators/block_allocator.h>
#include <cutl/allocators/fixed_size_allocator.h>
#include <cutl/allocators/stack_allocator.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// The library rounds the guard bytes up to the allocation alignment, so that the memory it hands out
// stays aligned however many guard bytes were asked for. Mirror that here to find them.
#if defined(CUTL_ALLOCATOR_ALIGNMENT)
#define TEST_ALIGNMENT CUTL_ALLOCATOR_ALIGNMENT
#else
#define TEST_ALIGNMENT alignof(max_align_t)
#endif
#define TEST_GUARD_BYTES ((CUTL_ALLOCATOR_GUARD_BYTES + TEST_ALIGNMENT - 1) & ~(size_t)(TEST_ALIGNMENT - 1))

enum : size_t
{
    BLOCK_SIZE = 256,
    BUFFER_SIZE = 128 * 1024,
};

/**
 * Check that memory is aligned the way every allocation in this library promises to be.
 *
 * @param memory Memory an allocation handed out.
 * @param size Size that was asked for.
 */
static void check_alignment(const void *const memory, const size_t size)
{
    TEST_ASSERTION(((uintptr_t)memory % TEST_ALIGNMENT) == 0,
                   "A block of %zu bytes came back at %p, which is not aligned to %zu bytes.", size, memory,
                   (size_t)TEST_ALIGNMENT);
}

/**
 * A parent allocator handing out memory aligned the way this build wants it. The standard allocator
 * only ever promises max_align_t, which is not enough once the library is built for a stricter
 * alignment than that, and the arena insists on being given memory it can line blocks up in.
 */
static void *aligned_allocate(void *const state, const size_t size)
{
    (void)state;
    return test_aligned_alloc(size);
}

static void aligned_deallocate(void *const state, void *const ptr)
{
    (void)state;
    free(ptr);
}

static void *aligned_reallocate(void *const state, void *const ptr, const size_t size)
{
    (void)state;
    return realloc(ptr, size);
}

static const cutl_allocator_t ALIGNED_PARENT = {
    .state = nullptr,
    .allocate = aligned_allocate,
    .deallocate = aligned_deallocate,
    .reallocate = aligned_reallocate,
};

/**
 * Check that every allocator hands out memory aligned to the allocation alignment.
 */
static void test_alignment(void)
{
    cutl_result_t result;
    for (size_t size = 1; size <= 1024; size *= 2)
    {
        cutl_result_t result;

        unsigned char *const stack_buffer = test_aligned_alloc(BUFFER_SIZE);
        TEST_ASSERTION(stack_buffer != nullptr, "Failed to allocate a buffer.");
        cutl_allocator_stack_t *stack;
        TEST_CUTL_RESULT(result, cutl_allocator_stack_create(BUFFER_SIZE, stack_buffer, &stack), CUTL_SUCCESS);
        void *const from_stack = cutl_alloc(cutl_allocator_stack_get(stack), size);
        TEST_ASSERTION(from_stack != nullptr, "The stack allocator failed to hand out %zu bytes.", size);
        check_alignment(from_stack, size);
        free(stack_buffer);

        unsigned char *const block_buffer = test_aligned_alloc(BUFFER_SIZE);
        TEST_ASSERTION(block_buffer != nullptr, "Failed to allocate a buffer.");
        cutl_allocator_block_t *blocks;
        TEST_CUTL_RESULT(result, cutl_allocator_block_create(BUFFER_SIZE, block_buffer, 1024, &blocks), CUTL_SUCCESS);
        void *const from_blocks = cutl_alloc(cutl_allocator_block_get(blocks), size);
        TEST_ASSERTION(from_blocks != nullptr, "The block allocator failed to hand out %zu bytes.", size);
        check_alignment(from_blocks, size);
        free(block_buffer);

        unsigned char *const fs_buffer = test_aligned_alloc(BUFFER_SIZE);
        TEST_ASSERTION(fs_buffer != nullptr, "Failed to allocate a buffer.");
        cutl_allocator_fs_t *fixed;
        TEST_CUTL_RESULT(result, cutl_allocator_fs_create(BUFFER_SIZE, fs_buffer, &fixed), CUTL_SUCCESS);
        void *const from_fixed = cutl_alloc(cutl_allocator_fs_get(fixed), size);
        TEST_ASSERTION(from_fixed != nullptr, "The fixed-size allocator failed to hand out %zu bytes.", size);
        check_alignment(from_fixed, size);
        free(fs_buffer);

        // The arena opts out of guards, but is still held to handing out aligned memory.
        cutl_allocator_arena_t *arena;
        TEST_CUTL_RESULT(result, cutl_allocator_arena_create(&ALIGNED_PARENT, 16 * 1024, &arena), CUTL_SUCCESS);
        void *const from_arena = cutl_alloc(cutl_allocator_arena_get(arena), size);
        TEST_ASSERTION(from_arena != nullptr, "The arena failed to hand out %zu bytes.", size);
        check_alignment(from_arena, size);
        cutl_allocator_arena_destroy(arena);
    }
}

#if CUTL_ALLOCATOR_GUARD_BYTES > 0

/**
 * Check that a write just past the end of an allocation is reported when the block is given back.
 */
static void test_rear_overflow_is_caught(void)
{
    cutl_result_t result;

    {
        unsigned char *const buffer = test_aligned_alloc(BUFFER_SIZE);
        cutl_allocator_stack_t *stack;
        TEST_CUTL_RESULT(result, cutl_allocator_stack_create(BUFFER_SIZE, buffer, &stack), CUTL_SUCCESS);
        void *block;
        TEST_CUTL_RESULT(result, cutl_allocator_stack_allocate(stack, BLOCK_SIZE, &block), CUTL_SUCCESS);
        // One byte past the end of what the caller was given lands in the rear guard.
        ((unsigned char *)block)[BLOCK_SIZE] = 0xFF;
        TEST_CUTL_RESULT(result, cutl_allocator_stack_deallocate(stack, block), CUTL_RESULT_CORRUPTED_POINTER);
        free(buffer);
    }

    {
        unsigned char *const buffer = test_aligned_alloc(BUFFER_SIZE);
        cutl_allocator_block_t *blocks;
        TEST_CUTL_RESULT(result, cutl_allocator_block_create(BUFFER_SIZE, buffer, 1024, &blocks), CUTL_SUCCESS);
        void *block;
        TEST_CUTL_RESULT(result, cutl_allocator_block_allocate(blocks, &block), CUTL_SUCCESS);
        ((unsigned char *)block)[cutl_allocator_block_get_block_size(blocks)] = 0xFF;
        TEST_CUTL_RESULT(result, cutl_allocator_block_deallocate(blocks, block), CUTL_RESULT_CORRUPTED_POINTER);
        free(buffer);
    }

    {
        unsigned char *const buffer = test_aligned_alloc(BUFFER_SIZE);
        cutl_allocator_fs_t *fixed;
        TEST_CUTL_RESULT(result, cutl_allocator_fs_create(BUFFER_SIZE, buffer, &fixed), CUTL_SUCCESS);
        void *block;
        TEST_CUTL_RESULT(result, cutl_allocator_fs_allocate(fixed, BLOCK_SIZE, &block), CUTL_SUCCESS);
        size_t usable = 0;
        TEST_CUTL_RESULT(result, cutl_allocator_fs_real_block_size(fixed, block, &usable), CUTL_SUCCESS);
        ((unsigned char *)block)[usable] = 0xFF;
        TEST_CUTL_RESULT(result, cutl_allocator_fs_deallocate(fixed, block), CUTL_RESULT_CORRUPTED_POINTER);
        free(buffer);
    }
}

/**
 * Check that a write just in front of an allocation is reported as well, which catches the blocks
 * running backwards rather than forwards.
 */
static void test_front_overflow_is_caught(void)
{
    cutl_result_t result;

    {
        unsigned char *const buffer = test_aligned_alloc(BUFFER_SIZE);
        cutl_allocator_stack_t *stack;
        TEST_CUTL_RESULT(result, cutl_allocator_stack_create(BUFFER_SIZE, buffer, &stack), CUTL_SUCCESS);
        void *block;
        TEST_CUTL_RESULT(result, cutl_allocator_stack_allocate(stack, BLOCK_SIZE, &block), CUTL_SUCCESS);
        // The first block the stack hands out has nothing but its front guard in front of it.
        ((unsigned char *)block)[-TEST_GUARD_BYTES] = 0xFF;
        TEST_CUTL_RESULT(result, cutl_allocator_stack_deallocate(stack, block), CUTL_RESULT_CORRUPTED_POINTER);
        free(buffer);
    }

    {
        unsigned char *const buffer = test_aligned_alloc(BUFFER_SIZE);
        cutl_allocator_block_t *blocks;
        TEST_CUTL_RESULT(result, cutl_allocator_block_create(BUFFER_SIZE, buffer, 1024, &blocks), CUTL_SUCCESS);
        void *block;
        TEST_CUTL_RESULT(result, cutl_allocator_block_allocate(blocks, &block), CUTL_SUCCESS);
        ((unsigned char *)block)[-TEST_GUARD_BYTES] = 0xFF;
        TEST_CUTL_RESULT(result, cutl_allocator_block_deallocate(blocks, block), CUTL_RESULT_CORRUPTED_POINTER);
        free(buffer);
    }

    {
        unsigned char *const buffer = test_aligned_alloc(BUFFER_SIZE);
        cutl_allocator_fs_t *fixed;
        TEST_CUTL_RESULT(result, cutl_allocator_fs_create(BUFFER_SIZE, buffer, &fixed), CUTL_SUCCESS);
        void *block;
        TEST_CUTL_RESULT(result, cutl_allocator_fs_allocate(fixed, BLOCK_SIZE, &block), CUTL_SUCCESS);
        ((unsigned char *)block)[-TEST_GUARD_BYTES] = 0xFF;
        TEST_CUTL_RESULT(result, cutl_allocator_fs_deallocate(fixed, block), CUTL_RESULT_CORRUPTED_POINTER);
        free(buffer);
    }
}

/**
 * Check that memory which was left alone is still given back without complaint, so that the checks
 * above are not simply reporting everything.
 */
static void test_intact_block_is_accepted(void)
{
    cutl_result_t result;
    unsigned char *const buffer = test_aligned_alloc(BUFFER_SIZE);
    cutl_allocator_stack_t *stack;
    TEST_CUTL_RESULT(result, cutl_allocator_stack_create(BUFFER_SIZE, buffer, &stack), CUTL_SUCCESS);
    void *block;
    TEST_CUTL_RESULT(result, cutl_allocator_stack_allocate(stack, BLOCK_SIZE, &block), CUTL_SUCCESS);
    memset(block, 0x5A, BLOCK_SIZE);
    TEST_CUTL_RESULT(result, cutl_allocator_stack_deallocate(stack, block), CUTL_SUCCESS);
    free(buffer);
}

#endif

int main(int argc, char *CUTL_ARRAY_ARG(argv, static argc))
{
    (void)argc;
    (void)argv;

    test_alignment();

#if CUTL_ALLOCATOR_GUARD_BYTES > 0
    test_intact_block_is_accepted();
    test_rear_overflow_is_caught();
    test_front_overflow_is_caught();
#else
    printf("Guard bytes are compiled out, so the overflow checks were skipped.\n");
#endif
    return 0;
}
