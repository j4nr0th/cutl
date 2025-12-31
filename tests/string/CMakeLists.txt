add_executable(test_format_integers ${CUTL_TEST_COMMMON_SOURCES} test_format_integers.c)
target_link_libraries(test_format_integers PRIVATE cutl)
add_test(NAME format_integers COMMAND test_format_integers)

add_executable(test_format_float ${CUTL_TEST_COMMMON_SOURCES} test_format_float.c)
target_link_libraries(test_format_float PRIVATE cutl)
add_test(NAME format_float COMMAND test_format_float)

add_executable(test_format_exponential ${CUTL_TEST_COMMMON_SOURCES} test_format_exponential.c)
target_link_libraries(test_format_exponential PRIVATE cutl)
add_test(NAME format_exponential COMMAND test_format_exponential)

add_executable(test_string_stream ${CUTL_TEST_COMMON_SOURCES} test_string_stream.c)
target_link_libraries(test_string_stream PRIVATE cutl)
add_test(NAME string_stream COMMAND test_string_stream)
