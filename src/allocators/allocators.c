#include "allocators.h"

enum : size_t
{
    STD_ALLOCATOR_MAGIC = 0xB160B00B1354BABE
};

#define CHECK_STD_MAGIC(state)                                                                                         \
    CUTL_ASSERT((size_t)state == STD_ALLOCATOR_MAGIC,                                                                  \
                "STD Allocator magic number did not match (expected %zX, but got %zX)", STD_ALLOCATOR_MAGIC,           \
                (size_t)state)

static void *std_allocate(void *const state, const size_t size)
{
    CHECK_STD_MAGIC(state);
    return malloc(size);
}

static void *std_reallocate(void *const state, void *const ptr, const size_t size)
{
    CHECK_STD_MAGIC(state);
    return realloc(ptr, size);
}

static void std_deallocate(void *const state, void *const ptr)
{
    CHECK_STD_MAGIC(state);
    free(ptr);
}

const cutl_allocator_t CUTL_STD_ALLOCATOR = {
    .state = (void *)STD_ALLOCATOR_MAGIC,
    .allocate = std_allocate,
    .deallocate = std_deallocate,
    .reallocate = std_reallocate,
};

static int GLOBAL_ALLOCATOR_SET = 0;
static cutl_allocator_t GLOBAL_ALLOCATOR;

static thread_local int THREAD_ALLOCATOR_SET = 0;
static thread_local cutl_allocator_t THREAD_ALLOCATOR;

void cutl_allocator_set_global(const cutl_allocator_t *allocator)
{
    if (!allocator)
    {
        GLOBAL_ALLOCATOR_SET = 0;
    }
    else
    {
        GLOBAL_ALLOCATOR_SET = 1;
        GLOBAL_ALLOCATOR = *allocator;
    }
}

void cutl_allocator_set_thread(const cutl_allocator_t *allocator)
{
    if (!allocator)
    {
        THREAD_ALLOCATOR_SET = 0;
    }
    else
    {
        THREAD_ALLOCATOR = *allocator;
        THREAD_ALLOCATOR_SET = 1;
    }
}

const cutl_allocator_t *cutl_allocator_get_default(void)
{
    if (THREAD_ALLOCATOR_SET)
        return &THREAD_ALLOCATOR;

    if (GLOBAL_ALLOCATOR_SET)
        return &GLOBAL_ALLOCATOR;

    return &CUTL_STD_ALLOCATOR;
}

void *cutl_alloc(const cutl_allocator_t *allocator, const size_t size)
{
    return allocator->allocate(allocator->state, size);
}

void *cutl_realloc(const cutl_allocator_t *allocator, void *ptr, const size_t new_size)
{
    if (ptr == nullptr)
        return cutl_alloc(allocator, new_size);
    return allocator->reallocate(allocator->state, ptr, new_size);
}

void cutl_dealloc(const cutl_allocator_t *allocator, void *ptr)
{
    if (ptr != nullptr)
        allocator->deallocate(allocator->state, ptr);
}

void *cutl_alloc_default(const size_t size)
{
    return cutl_alloc(cutl_allocator_get_default(), size);
}

void *cutl_realloc_default(void *ptr, const size_t new_size)
{
    return cutl_realloc(cutl_allocator_get_default(), ptr, new_size);
}

void cutl_dealloc_default(void *ptr)
{
    cutl_dealloc(cutl_allocator_get_default(), ptr);
}
