#include "allocator_common_test.h"
#include <cutl/allocators/stack_allocator.h>
#include <string.h>
#include <time.h>

enum : size_t
{
    ALLOCATION_MIN_SIZE = 32,
    ALLOCATION_MAX_SIZE = 2049,
};

static void take_difference(struct timespec *end, const struct timespec *start)
{
    if (end->tv_nsec < start->tv_nsec)
    {
        end->tv_nsec += 1000000000;
        end->tv_sec -= 1;
    }
    end->tv_sec -= start->tv_sec;
    end->tv_nsec -= start->tv_nsec;
}

static void time_allocator(const unsigned cnt, const cutl_allocator_t *const allocator)
{
    struct timespec start, end1, end2, end3, end4;
    enum
    {
        CLOCK_ID = CLOCK_PROCESS_CPUTIME_ID
    };
    clock_gettime(CLOCK_ID, &start);
    test_allocator(allocator, cnt, ALLOCATION_MIN_SIZE, ALLOCATION_MAX_SIZE, 235723509, 10);
    clock_gettime(CLOCK_ID, &end1);
    test_allocator(allocator, cnt, ALLOCATION_MIN_SIZE, ALLOCATION_MAX_SIZE, 6578578, 10);
    clock_gettime(CLOCK_ID, &end2);
    test_allocator(allocator, cnt * 2, (ALLOCATION_MIN_SIZE + 1) / 2, ALLOCATION_MAX_SIZE / 2, 28248, 10);
    clock_gettime(CLOCK_ID, &end3);
    test_allocator(allocator, cnt / 2, ALLOCATION_MIN_SIZE * 2, ALLOCATION_MAX_SIZE * 2, 5443254, 10);
    clock_gettime(CLOCK_ID, &end4);

    take_difference(&end4, &end3);
    take_difference(&end3, &end2);
    take_difference(&end2, &end1);
    take_difference(&end1, &start);

    printf("Times taken: %ld.%09ld, %ld.%09ld, %ld.%09ld, %ld.%09ld; total %ld.%09ld \n", end1.tv_sec, end1.tv_nsec,
           end2.tv_sec, end2.tv_nsec, end3.tv_sec, end3.tv_nsec, end4.tv_sec, end4.tv_nsec,
           (end1.tv_sec + end2.tv_sec + end3.tv_sec + end4.tv_sec),
           (end1.tv_nsec + end2.tv_nsec + end3.tv_nsec + end4.tv_nsec));
}

/**
 * Check that growing the block sitting on top of the stack keeps it where it is, and that the stack
 * accounts for the memory it just took. The block is grown into memory nothing else has been handed
 * out of yet, so moving it would be pointless.
 */
static void test_grow_in_place(void)
{
    enum
    {
        STACK_SIZE = 256 * 1024,
    };
    unsigned char *const memory = test_aligned_alloc(STACK_SIZE);
    TEST_ASSERTION(memory != nullptr, "Failed to allocate the stack's memory.");
    cutl_result_t result;
    cutl_allocator_stack_t *stack;
    TEST_CUTL_RESULT(result, cutl_allocator_stack_create(STACK_SIZE, memory, &stack), CUTL_SUCCESS);
    auto const interface = cutl_allocator_stack_get(stack);

    void *const block = cutl_alloc(interface, 128);
    TEST_ASSERTION(block != nullptr, "Failed to allocate a block.");
    memset(block, 0x3C, 128);
    auto const top_before = cutl_allocator_stack_get_top(stack);

    // Nothing was allocated after this block, so it is on top of the stack and can grow there.
    void *grown;
    TEST_CUTL_RESULT(result, cutl_allocator_stack_reallocate(stack, block, 4096, &grown), CUTL_SUCCESS);
    TEST_ASSERTION(grown == block, "Growing the block on top of the stack moved it.");
    TEST_ASSERTION(((unsigned char *)grown)[0] == 0x3C && ((unsigned char *)grown)[127] == 0x3C,
                   "Growing the block on top of the stack lost its contents.");
    // The stack has to know it handed out more, or the next allocation lands on top of this one.
    TEST_ASSERTION(cutl_allocator_stack_get_top(stack) > top_before,
                   "The stack top stayed at %zu after the block grew from it.", cutl_allocator_stack_get_top(stack));

    // Whatever comes next must not overlap the block we just grew.
    void *const next = cutl_alloc(interface, 128);
    TEST_ASSERTION(next != nullptr, "Failed to allocate a block after growing one.");
    TEST_ASSERTION((unsigned char *)next >= (unsigned char *)grown + 4096,
                   "A block was handed out at %p, which is inside the block grown at %p.", next, grown);

    // With something allocated after it, the block is no longer on top and has to move.
    void *moved;
    TEST_CUTL_RESULT(result, cutl_allocator_stack_reallocate(stack, grown, 8192, &moved), CUTL_SUCCESS);
    TEST_ASSERTION(moved != grown, "Growing a block with another block after it did not move it.");
    TEST_ASSERTION(((unsigned char *)moved)[0] == 0x3C && ((unsigned char *)moved)[127] == 0x3C,
                   "Moving a block lost its contents.");

    free(memory);
}

int main(const int argc, const char *CUTL_ARRAY_ARG(argv, const static argc))
{
    test_grow_in_place();

    TEST_ASSERTION(argc == 2, "Two arguments were expected.");
    char *end_ptr;
    auto const cnt = strtoul(argv[1], &end_ptr, 10);
    TEST_ASSERTION(end_ptr != argv[1], "Parameter was not a positive integer.");
    TEST_ASSERTION(cnt > 0, "Parameter was not a positive integer.");

    auto const per_block = (size_t)ALLOCATION_MAX_SIZE + 3 * (size_t)TEST_ALLOCATOR_ALIGNMENT;
    auto const total_required_memory = cnt * 2 * per_block + 4 * (size_t)TEST_ALLOCATOR_ALIGNMENT;

    unsigned char *const memory = test_aligned_alloc(total_required_memory);
    TEST_ASSERTION(memory != nullptr, "Failed to allocate memory.");
    cutl_allocator_stack_t *allocator;
    auto const res = cutl_allocator_stack_create(total_required_memory, memory, &allocator);
    TEST_ASSERTION(res == CUTL_SUCCESS, "Failed to create allocator: (%s) - %s.", cutl_result_to_string(res),
                   cutl_result_message(res));
    auto const allocator_fs = cutl_allocator_stack_get(allocator);

    time_allocator(cnt, allocator_fs);
    time_allocator(cnt, cutl_allocator_get_default());

    free(memory);
    return 0;
}
