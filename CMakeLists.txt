cmake_minimum_required(VERSION 4.1)
project(cutl C)

set(CMAKE_C_STANDARD 23)

set(CUTL_ITERATOR_SOURCES
        src/iterators/multidim_iteration.c
        src/iterators/permutation_iterator.c
        src/iterators/combination_iterator.c
)

set(CUTL_ALLOCATOR_SOURCES
        src/allocators/allocators.c
        src/allocators/fixed_size_allocator.c
        src/allocators/stack_allocator.c
        src/allocators/block_allocator.c
        src/allocators/os_allocator.c
)

set(CUTL_STRING_SOURCES
        src/string/strings.c
        src/string/format.c
        src/string/output_stream.c
        src/string/string_stream.c
        src/string/format_defaults.c
)

set(CUTL_COMMON_SOURCES
        src/error.c
)


add_library(cutl STATIC
        ${CUTL_COMMON_SOURCES}
        ${CUTL_ITERATOR_SOURCES}
        ${CUTL_ALLOCATOR_SOURCES}
        ${CUTL_STRING_SOURCES}
        ${CUTL_THREAD_SOURCES}
)
target_link_libraries(cutl PUBLIC m)
target_include_directories(cutl INTERFACE ${CMAKE_SOURCE_DIR}/include)

target_compile_definitions(cutl PUBLIC CUTL_ENABLE_ASSERTS)
target_compile_options(cutl PRIVATE -Wall -Wextra)

add_subdirectory(tests)
