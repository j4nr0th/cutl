cmake_minimum_required(VERSION 4.1)
project(cutl C)

set(CMAKE_C_STANDARD 23)

list(APPEND CUTL_SRC_FILES
        src/iterators/multidim_iteration.c
        src/iterators/permutations.c
        src/allocators/allocators.c
        src/allocators/fixed_size_allocator.c
        src/error.c
)
add_library(cutl STATIC ${CUTL_SRC_FILES}
        tests/allocators/test_fixed_size_allocator.c
)

target_compile_definitions(cutl PUBLIC CUTL_ENABLE_ASSERTS)
target_compile_options(cutl PRIVATE -Wall -Wextra)

enable_testing()
# target_compile_definitions(cutl PUBLIC CUTL_VALIDATE_ALLOCATORS)
add_subdirectory(tests)
