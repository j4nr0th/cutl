#include "../../src/allocators/fixed_size_allocator.h"
#include "allocator_common_test.h"
// #include <time.h>

enum : size_t
{
    ALLOCATION_MIN_SIZE = 1,
    ALLOCATION_MAX_SIZE = 67,
};

static void time_allocator(const unsigned cnt, const cutl_allocator_t *const allocator)
{
    // struct timespec start, end;
    // enum {CLOCK_ID = CLOCK_PROCESS_CPUTIME_ID};
    // clock_gettime(CLOCK_ID, &start);
    test_allocator(allocator, cnt, ALLOCATION_MIN_SIZE, ALLOCATION_MAX_SIZE, 235723509, 10);
    test_allocator(allocator, cnt, ALLOCATION_MIN_SIZE, ALLOCATION_MAX_SIZE, 6578578, 10);
    test_allocator(allocator, cnt * 2, ALLOCATION_MIN_SIZE, ALLOCATION_MAX_SIZE / 2, 28248, 10);
    test_allocator(allocator, cnt, ALLOCATION_MIN_SIZE * 2, ALLOCATION_MAX_SIZE, 5443254, 10);
    // clock_gettime(CLOCK_ID, &end);
    // if (end.tv_nsec < start.tv_nsec)
    // {
    //     end.tv_nsec += 1000000000;
    //     end.tv_sec -= 1;
    // }
    // printf("Time taken: %ld.%09ld\n", end.tv_sec - start.tv_sec, end.tv_nsec - start.tv_nsec);
}

int main(const int argc, const char *CUTL_ARRAY_ARG(argv, const static argc))
{
    TEST_ASSERTION(argc == 2, "Two arguments were expected.");
    char *end_ptr;
    auto const cnt = strtoul(argv[1], &end_ptr, 10);
    TEST_ASSERTION(end_ptr != argv[1], "Parameter was not a positive integer.");
    TEST_ASSERTION(cnt > 0, "Parameter was not a positive integer.");

    auto const total_required_memory =
        cnt * 2 * (ALLOCATION_MAX_SIZE) + (cnt + 1) * (sizeof(memory_block_info_t) + sizeof(cutl_allocator_fs_t));

    unsigned char *const memory = malloc(total_required_memory);
    TEST_ASSERTION(memory != nullptr, "Failed to allocate memory.");
    cutl_allocator_fs_t *allocator;
    auto const res = cutl_allocator_fs_create(total_required_memory, memory, &allocator);
    TEST_ASSERTION(res == CUTL_SUCCESS, "Failed to create allocator: (%s) - %s.", cutl_result_to_string(res),
                   cutl_result_message(res));
    auto const allocator_fs = cutl_allocator_fs_get(allocator);
    // auto const allocator_fs = cutl_allocator_get_default();
    time_allocator(cnt, allocator_fs);
    // time_allocator(cnt, cutl_allocator_get_default());

    free(memory);
    return 0;
}
