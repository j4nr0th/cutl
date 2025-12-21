list(APPEND CUTL_TEST_COMMMON_SOURCES
    ${PROJECT_SOURCE_DIR}/tests/common/common.c
)
enable_testing()
add_subdirectory(iterators)
add_subdirectory(allocators)
