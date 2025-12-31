#include "../../src/iterators/combinations.h"
#include "../common/common.h"

#include <stddef.h>
#include <string.h>

// static void print_combination(const combination_iterator_t *p)
// {
//     const unsigned char *const val = combination_iterator_current(p);
//     printf("%hhd", val[0]);
//     for (unsigned i = 1; i < p->r; ++i)
//         printf(" %hhd", val[i]);
// }

static int are_combinations_equal(const unsigned r, const unsigned char a[static r], const unsigned char b[static r])
{
    unsigned matching_count = 0;

    for (unsigned i = 0; i < r; ++i)
    {
        for (unsigned j = i + 1; j < r; ++j)
            matching_count += a[i] == b[j];
    }

    return matching_count >= r;
}

static void test_combinations(const unsigned char n, const unsigned char r)
{
    printf("Testing n: %u r: %u\n", (unsigned)n, (unsigned)r);
    TEST_ASSERTION(n >= r,
                   "Number of elements must be greater than or equal to the number of elements per combination.");
    combination_iterator_t *const p = malloc(combination_iterator_required_memory(r));
    TEST_ASSERTION(p, "Failed to allocate combination iterator.");
    combination_iterator_init(p, n, r);
    const unsigned total_combinations = combination_iterator_total_count(p);
    unsigned cnt = 0;
    unsigned char *const previous_combinations = malloc((size_t)r * total_combinations);
    TEST_ASSERTION(previous_combinations, "Failed to allocate memory for previous combinations.");
    while (!combination_iterator_is_done(p))
    {
        // Copy the current iteration to the buffer
        const unsigned char *const current_combination = combination_iterator_current(p);
        memcpy(previous_combinations + (size_t)cnt * r, current_combination, r);
        for (unsigned i = 0; i < r; ++i)
        {
            auto const v = current_combination[i];
            for (unsigned j = i + 1; j < r; ++j)
                TEST_ASSERTION(current_combination[j] != v, "Combination should not contain duplicate elements.");
        }

        // printf("combination %u: ", cnt + 1);
        // print_combination(p);
        // printf("\n");

        // Check the current iteration does not repeat!
        for (unsigned i = 0; i < cnt; ++i)
        {
            TEST_ASSERTION(!are_combinations_equal(r, current_combination, previous_combinations + (size_t)(i * r)),
                           "combination should not repeat, but combination %u and %u are the same.", cnt + 1, i + 1);
        }

        cnt += 1;
        combination_iterator_next(p);
    }

    TEST_ASSERTION(cnt == total_combinations, "Wrong number of combinations generated (expected %u, but only got %u).",
                   total_combinations, cnt);

    free(previous_combinations);
    free(p);
    printf("Finished n: %u r: %u\n", (unsigned)n, (unsigned)r);
}

int main(void)
{
    // test_combinations(3, 0);
    test_combinations(3, 1);
    test_combinations(3, 2);
    test_combinations(5, 2);
    test_combinations(5, 5);
    test_combinations(10, 3);
}
