#pragma once
#include "../common_defs.h"

typedef struct
{
    void *state;
    void *(*allocate)(void *state, size_t size);
    void (*deallocate)(void *state, void *ptr);
    void *(*reallocate)(void *state, void *ptr, size_t new_size);
} cutl_allocator_t;

extern const cutl_allocator_t CUTL_STD_ALLOCATOR;

void cutl_allocator_set_global(const cutl_allocator_t *allocator);

void cutl_allocator_set_thread(const cutl_allocator_t *allocator);

const cutl_allocator_t *cutl_allocator_get_default(void);

void *cutl_alloc(const cutl_allocator_t *allocator, size_t size);

void *cutl_realloc(const cutl_allocator_t *allocator, void *ptr, size_t new_size);

void cutl_dealloc(const cutl_allocator_t *allocator, void *ptr);

void *cutl_alloc_default(size_t size);

void *cutl_realloc_default(void *ptr, size_t new_size);

void cutl_dealloc_default(void *ptr);

