cmake_minimum_required(VERSION 4.1)
project(cutl C)

set(CMAKE_C_STANDARD 23)

set(CUTL_ITERATOR_SOURCES
        src/iterators/multidim_iteration.c
        src/iterators/permutations.c
        src/iterators/multidim_iteration.c
        src/iterators/permutations.c
)

set(CUTL_ALLOCATOR_SOURCES
        src/allocators/allocators.c
        src/allocators/fixed_size_allocator.c
        src/allocators/stack_allocator.c
        src/allocators/block_allocator.c
        src/allocators/os_allocator.c
)

set(CUTL_STRING_SOURCES
        src/string/string8.c
        src/string/format8.c
        src/string/format8.h
)

set(CUTL_COMMON_SOURCES
        src/error.c
)


add_library(cutl STATIC
        ${CUTL_COMMON_SOURCES}
        ${CUTL_ITERATOR_SOURCES}
        ${CUTL_ALLOCATOR_SOURCES}
        ${CUTL_STRING_SOURCES}
)
target_link_libraries(cutl PUBLIC m)

target_compile_definitions(cutl PUBLIC CUTL_ENABLE_ASSERTS)
target_compile_options(cutl PRIVATE -Wall -Wextra)

enable_testing()
# target_compile_definitions(cutl PUBLIC CUTL_VALIDATE_ALLOCATORS)
add_subdirectory(tests)
