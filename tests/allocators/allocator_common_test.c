#include "allocator_common_test.h"

#include <string.h>

// TODO: delet dis
#include "../../src/allocators/fixed_size_allocator.h"

static void permute_index_array(test_prng_t *rng, const unsigned size,
                                unsigned CUTL_ARRAY_ARG(array, const static size))
{
    for (unsigned i = 0; i < size; ++i)
    {
        const unsigned j = test_prng_next_uint(rng) % size;
        const unsigned tmp = array[i];
        array[i] = array[j];
        array[j] = tmp;
    }
}

void test_allocator(const cutl_allocator_t *const allocator, const unsigned allocations, const size_t min_alloc_size,
                    const size_t max_alloc_size, const unsigned seed, const unsigned permutation_rounds)
{
    test_prng_t rng;
    test_prng_seed(&rng, seed);

    typedef struct
    {
        void *ptr;
        size_t first_size;
        size_t second_size;
        unsigned char fill;
    } allocation;

    allocation *const allocations_info = malloc(allocations * sizeof(allocation));
    TEST_ASSERTION(allocations_info, "Failed to allocate allocation info.");
    unsigned *const indices = malloc(allocations * sizeof(unsigned));
    TEST_ASSERTION(indices, "Failed to allocate indices.");
    for (unsigned i = 0; i < allocations; ++i)
    {
        indices[i] = i;
    }
    // Permute it very well
    for (unsigned i = 0; i < permutation_rounds; ++i)
        permute_index_array(&rng, allocations, indices);

    // Create new allocations and fill them
    for (unsigned i = 0; i < allocations; ++i)
    {
        const unsigned idx = indices[i];
        auto const info = allocations_info + idx;

        const unsigned char fill = (unsigned char)test_prng_next_uint(&rng);
        const size_t size = test_prng_next_uint(&rng) % (max_alloc_size - min_alloc_size + 1) + min_alloc_size;
        info->ptr = cutl_alloc(allocator, size);
        info->first_size = size;
        info->fill = fill;
        TEST_ASSERTION(info->ptr, "Failed to allocate memory.");
        memset(info->ptr, fill, size);
    }

    // Check all the allocations were fine
    for (unsigned i = 0; i < allocations; ++i)
    {
        auto const info = allocations_info + i;
        for (unsigned char *ptr = info->ptr, *end = ptr + info->first_size; ptr < end; ++ptr)
        {
            TEST_ASSERTION(*ptr == info->fill,
                           "Memory was not filled correctly (allocation %u had value %hhx at byte %zu instead of %hhx.",
                           i, *ptr, ptr - (unsigned char *)info->ptr, info->fill);
        }
    }

    // Once again, permute the index array
    for (unsigned i = 0; i < permutation_rounds; ++i)
        permute_index_array(&rng, allocations, indices);
    // Reallocate all the allocations
    for (unsigned i = 0; i < allocations; ++i)
    {
        const unsigned idx = indices[i];

        auto const info = allocations_info + idx;
        const size_t new_size = test_prng_next_uint(&rng) % (max_alloc_size - min_alloc_size + 1) + min_alloc_size;
        info->ptr = cutl_realloc(allocator, info->ptr, new_size);
        TEST_ASSERTION(info->ptr, "Failed to reallocate memory.");
        size_t real_size;
        TEST_ASSERTION(cutl_allocator_fs_real_block_size((const cutl_allocator_fs_t *)allocator, info->ptr,
                                                         &real_size) == CUTL_SUCCESS,
                       "Could not get real size of the block.");
        TEST_ASSERTION(real_size >= new_size, "Reallocated block was not the correct size (%zu vs %zu) !.", real_size,
                       new_size);
        info->second_size = new_size;
        // Check the reallocations
        for (unsigned j = 0; j < allocations; ++j)
        {
            auto const j_info = allocations_info + j;
            size_t real_size_2;
            TEST_ASSERTION(cutl_allocator_fs_real_block_size((const cutl_allocator_fs_t *)allocator, j_info->ptr,
                                                             &real_size_2) == CUTL_SUCCESS,
                           "Could not get real size of the block.");
            TEST_ASSERTION(real_size_2 >= j_info->second_size, "Reallocated block was not the correct size (%zu vs %zu) !.",
                           real_size_2, j_info->second_size);
        }
    }

    // Check all the necessary memory was moved
    for (unsigned i = 0; i < allocations; ++i)
    {
        auto const info = allocations_info + i;
        const size_t min_size = info->first_size < info->second_size ? info->first_size : info->second_size;
        for (unsigned char *ptr = info->ptr, *end = info->ptr + min_size; ptr < end; ++ptr)
        {
            TEST_ASSERTION(*ptr == info->fill,
                           "Memory was not moved correctly (allocation %u had value %hhx at byte %zu instead of %hhx.",
                           i, *ptr, ptr - (unsigned char *)info->ptr, info->fill);
        }
    }

    // For fun, permute the index array again
    for (unsigned i = 0; i < permutation_rounds; ++i)
        permute_index_array(&rng, allocations, indices);

    // Fill the memory with new fills
    for (unsigned i = 0; i < allocations; ++i)
    {
        const unsigned char new_fill = (unsigned char)test_prng_next_uint(&rng);
        const auto info = allocations_info + indices[i];
        memset(info->ptr, new_fill, info->second_size);
        info->fill = new_fill;
        for (const unsigned char *ptr = info->ptr, *const end = ptr + info->second_size; ptr < end; ++ptr)
        {
            TEST_ASSERTION(*ptr == info->fill,
                           "Memory was not filled correctly (allocation %u had value %hhx at byte %zu instead of %hhx.",
                           i, *ptr, ptr - (unsigned char *)info->ptr, info->fill);
        }

        // Check the fills are there
        for (unsigned j = 0; j < i; ++j)
        {
            const auto p_info = allocations_info + indices[j];
            for (const unsigned char *ptr = p_info->ptr, *const end = ptr + p_info->second_size; ptr < end; ++ptr)
            {
                TEST_ASSERTION(
                    *ptr == p_info->fill,
                    "Memory was not filled correctly (allocation %u had value %hhx at byte %zu instead of %hhx.", j,
                    *ptr, ptr - (unsigned char *)p_info->ptr, p_info->fill);
            }
        }
    }

    // Check the fills are there
    for (unsigned i = 0; i < allocations; ++i)
    {
        const auto info = allocations_info + i;
        for (const unsigned char *ptr = info->ptr, *const end = ptr + info->second_size; ptr < end; ++ptr)
        {
            TEST_ASSERTION(*ptr == info->fill,
                           "Memory was not filled correctly (allocation %u had value %hhx at byte %zu instead of %hhx.",
                           i, *ptr, ptr - (unsigned char *)info->ptr, info->fill);
        }
    }

    // We are done, deallocate memory in a random way!
    // For fun, permute the index for the last time
    for (unsigned i = 0; i < permutation_rounds; ++i)
        permute_index_array(&rng, allocations, indices);

    for (unsigned i = 0; i < allocations; ++i)
    {
        auto const info = allocations_info + indices[i];
        cutl_dealloc(allocator, info->ptr);
        *info = (allocation){};
    }

    free(indices);
    free(allocations_info);
}