list(APPEND CUTL_ALLOCATOR_TEST_FILES allocator_common_test.c ${CUTL_TEST_COMMMON_SOURCES})

add_executable(test_fixed_size_allocator ${CUTL_ALLOCATOR_TEST_FILES} test_fixed_size_allocator.c)
target_link_libraries(test_fixed_size_allocator PRIVATE cutl)
add_test(NAME fs_allocator COMMAND test_fixed_size_allocator 123)

add_executable(test_stack_allocator ${CUTL_ALLOCATOR_TEST_FILES} test_stack_allocator.c)
target_link_libraries(test_stack_allocator PRIVATE cutl)
add_test(NAME stack_allocator COMMAND test_stack_allocator 123)

add_executable(test_block_allocator ${CUTL_ALLOCATOR_TEST_FILES} test_block_allocator.c)
target_link_libraries(test_block_allocator PRIVATE cutl)
add_test(NAME block_allocator COMMAND test_block_allocator 123)

