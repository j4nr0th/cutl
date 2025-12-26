add_executable(test_format_integers ${CUTL_TEST_COMMMON_SOURCES}  test_format_integers.c)
target_link_libraries(test_format_integers PRIVATE cutl)
add_test(NAME format_integers COMMAND test_format_integers)
