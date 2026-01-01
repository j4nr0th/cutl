#include "../../include/cutl/allocators/os_allocator.h"
#include "allocator_internal.h"

#if __has_include(<sys/mman.h>)
#    include <sys/mman.h>
#    include <unistd.h>
#    define USE_MMAP
#    define FOUND_API
#endif

#ifndef FOUND_API
#    error No OS allocator API was found
#endif

static constexpr uintptr_t OS_ALLOCATOR_MAGIC = 0xAB16BADBABE;

#define CHECK_OS_MAGIC(state)                                                                                          \
    CUTL_ASSERT((uintptr_t)state == OS_ALLOCATOR_MAGIC, "OS Allocator magic number did not match")

typedef struct
{
    size_t size;
    alignas(max_align_t) unsigned char memory[];
} mapping_info_t;

static void *prepare_block(void *ptr, const size_t size)
{
    auto const info = (mapping_info_t *)ptr;
    info->size = size;
    // _prepare_block_used(size - sizeof(*info), info->memory);
    return info->memory;
}

static mapping_info_t *recover_memory_block(void *ptr)
{
    mapping_info_t *const info = (mapping_info_t *)((uintptr_t)ptr - sizeof(*info));
    return info;
}

#ifdef USE_MMAP

static size_t OS_PAGE_SIZE = 0;

static size_t ensure_page_size(void)
{
    if (OS_PAGE_SIZE == 0)
    {
        OS_PAGE_SIZE = sysconf(_SC_PAGE_SIZE);
    }
    return OS_PAGE_SIZE;
}

static size_t round_block_size(const size_t size)
{
    auto const needed_size = (size + sizeof(mapping_info_t));
    auto const page_size = ensure_page_size();
    auto const rem = needed_size % page_size;
    return needed_size + (rem != 0 ? page_size - rem : 0);
}

static void *allocate_memory(void *state, const size_t size)
{
    CHECK_OS_MAGIC(state);
    if (!size)
        return nullptr;
    auto const rounded_size = round_block_size(size);
    auto const ptr = mmap(nullptr, rounded_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (!ptr)
        return nullptr;
    return prepare_block(ptr, size);
}

static void deallocate_memory(void *state, void *ptr)
{
    CHECK_OS_MAGIC(state);
    auto const info = recover_memory_block(ptr);
    munmap(info, 0);
}

static void *reallocate_memory(void *state, void *ptr, const size_t size)
{
    if (!ptr)
        return allocate_memory(state, size);
    if (size == 0)
    {
        return nullptr;
    }

    auto const info = recover_memory_block(ptr);
    auto const old_useful_size = info->size - sizeof(*info);
    if (old_useful_size >= size)
        return ptr;

    auto const rounded_size = round_block_size(size);
    auto new_ptr = mmap(nullptr, rounded_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (!new_ptr)
        return nullptr;

    new_ptr = prepare_block(new_ptr, rounded_size);
    auto const copy_size = size < old_useful_size ? size : old_useful_size;
    memcpy(new_ptr, ptr, copy_size);
    return new_ptr;
}

#endif

const cutl_allocator_t OS_ALLOCATOR = {
    .state = (void *)OS_ALLOCATOR_MAGIC,
    .allocate = allocate_memory,
    .deallocate = deallocate_memory,
    .reallocate = reallocate_memory,
};
