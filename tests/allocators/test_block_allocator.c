#include "../../src/allocators/block_allocator.h"
#include "allocator_common_test.h"
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
    struct timespec start, end1, end2, end3;
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

    take_difference(&end3, &end2);
    take_difference(&end2, &end1);
    take_difference(&end1, &start);

    printf("Times taken: %ld.%09ld, %ld.%09ld, %ld.%09ld; total %ld.%09ld \n", end1.tv_sec, end1.tv_nsec, end2.tv_sec,
           end2.tv_nsec, end3.tv_sec, end3.tv_nsec, (end1.tv_sec + end2.tv_sec + end3.tv_sec),
           (end1.tv_nsec + end2.tv_nsec + end3.tv_nsec));
}

int main(const int argc, const char *CUTL_ARRAY_ARG(argv, const static argc))
{
    TEST_ASSERTION(argc == 2, "Two arguments were expected.");
    char *end_ptr;
    auto const cnt = strtoul(argv[1], &end_ptr, 10);
    TEST_ASSERTION(end_ptr != argv[1], "Parameter was not a positive integer.");
    TEST_ASSERTION(cnt > 0, "Parameter was not a positive integer.");

    auto const total_required_memory = 2 * cnt * (ALLOCATION_MAX_SIZE + 32 + 16) + sizeof(cutl_allocator_block_t);

    unsigned char *const memory = malloc(total_required_memory);
    TEST_ASSERTION(memory != nullptr, "Failed to allocate memory.");
    cutl_allocator_block_t *allocator;
    auto const res = cutl_allocator_block_create(total_required_memory, memory, ALLOCATION_MAX_SIZE, &allocator);
    TEST_ASSERTION(res == CUTL_SUCCESS, "Failed to create allocator: (%s) - %s.", cutl_result_to_string(res),
                   cutl_result_message(res));
    auto const allocator_fs = cutl_allocator_block_get(allocator);

    time_allocator(cnt, allocator_fs);
    time_allocator(cnt, cutl_allocator_get_default());

    free(memory);
    return 0;
}
