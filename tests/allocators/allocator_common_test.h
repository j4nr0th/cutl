#pragma once
#include <cutl/allocators.h>
#include "../common/common.h"

/**
 * Test function, which checks if the allocator can properly allocate, reallocate, and deallocate.
 *
 * @param allocator Allocator to test.
 * @param allocations The number of allocations to make for this test.
 * @param min_alloc_size Smallest size of allocations to make.
 * @param max_alloc_size Largest size of allocations to make.
 * @param seed Seed of the pseudo-rng.
 * @param permutation_rounds Number of permutation rounds to shuffle indices with.
 */
void test_allocator(const cutl_allocator_t *allocator, unsigned allocations, size_t min_alloc_size,
                    size_t max_alloc_size, unsigned seed, unsigned permutation_rounds);
