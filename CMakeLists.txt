cmake_minimum_required(VERSION 3.15)
project(cutl C)

set(CMAKE_C_STANDARD 23)

add_subdirectory(cutf)

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
        src/allocators/arena_allocator.c
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

# Compile-time options.
#
# Each one is only given a default when the including project did not set it already, so that a
# project pulling this one in with add_subdirectory() stays in charge of them, whether it sets them
# as normal or as cache variables beforehand.

# Alignment every allocation is handed out with. Zero leaves the library to use its own default of
# max_align_t, which is the alignment a single allocation can be relied upon to have.
if(NOT DEFINED CUTL_ALLOCATOR_ALIGNMENT)
    set(CUTL_ALLOCATOR_ALIGNMENT 0)
endif()

# Guard bytes placed on both ends of every allocation. Guards are what catch a write outside of an
# allocation, so they are only left out of a build where nothing is expected to do that.
if(NOT DEFINED CUTL_ALLOCATOR_GUARD_BYTES)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(CUTL_ALLOCATOR_GUARD_BYTES 0)
    else()
        set(CUTL_ALLOCATOR_GUARD_BYTES 16)
    endif()
endif()

# The internal consistency checks. Exposed separately from the guards, as they are worth keeping in
# a release build, where the guards are usually off.
if(NOT DEFINED CUTL_ASSERTS)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(CUTL_ASSERTS 0)
    else()
        set(CUTL_ASSERTS 1)
    endif()
endif()

# Exhaustive, and quadratic, checking of the allocators' own bookkeeping. For pinning down a
# suspected allocator bug only.
if(NOT DEFINED CUTL_VALIDATE_ALLOCATORS)
    set(CUTL_VALIDATE_ALLOCATORS 0)
endif()

option(CUTL_BUILD_TESTS "Build the cutl test suite" ON)

# The tests are what catch a write outside an allocation, so they run with the guard bytes they ask
# for, whatever the default for this build type happens to be. This has to be settled before the
# library is put together, as the guards are compiled into it. Setting it to 0 is how the no-guard
# build gets covered as well.
if(NOT DEFINED CUTL_TEST_GUARD_BYTES)
    set(CUTL_TEST_GUARD_BYTES 16)
endif()
if(CUTL_BUILD_TESTS)
    set(CUTL_ALLOCATOR_GUARD_BYTES ${CUTL_TEST_GUARD_BYTES})
endif()

add_library(cutl STATIC
        ${CUTL_COMMON_SOURCES}
        ${CUTL_ITERATOR_SOURCES}
        ${CUTL_ALLOCATOR_SOURCES}
        ${CUTL_STRING_SOURCES}
        ${CUTL_THREAD_SOURCES}
)
target_link_libraries(cutl PUBLIC m cutf)
target_include_directories(cutl INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(CUTL_ASSERTS)
    target_compile_definitions(cutl PUBLIC CUTL_ENABLE_ASSERTS)
endif()
target_compile_definitions(cutl PUBLIC CUTL_ALLOCATOR_GUARD_BYTES=${CUTL_ALLOCATOR_GUARD_BYTES})
if(CUTL_ALLOCATOR_ALIGNMENT GREATER 0)
    target_compile_definitions(cutl PUBLIC CUTL_ALLOCATOR_ALIGNMENT=${CUTL_ALLOCATOR_ALIGNMENT})
endif()
if(CUTL_VALIDATE_ALLOCATORS)
    target_compile_definitions(cutl PUBLIC CUTL_VALIDATE_ALLOCATORS)
endif()
target_compile_options(cutl PRIVATE -Wall -Wextra)

if(CUTL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
