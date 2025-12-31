add_executable(test_multidim_1d ${CUTL_TEST_COMMMON_SOURCES} test_multidim_iterator_1d.c)
target_link_libraries(test_multidim_1d PRIVATE cutl)
add_test(NAME multidim_1d COMMAND test_multidim_1d 120)

add_executable(test_multidim_2d ${CUTL_TEST_COMMMON_SOURCES} test_multidim_iterator_2d.c)
target_link_libraries(test_multidim_2d PRIVATE cutl)
add_test(NAME multidim_2d COMMAND test_multidim_2d 12 40)

add_executable(test_multidim_3d ${CUTL_TEST_COMMMON_SOURCES} test_multidim_iterator_3d.c)
target_link_libraries(test_multidim_3d PRIVATE cutl)
add_test(NAME multidim_3d COMMAND test_multidim_3d 13 7 17)

add_executable(test_perm ${CUTL_TEST_COMMON_SOURCES} test_permutations.c)
target_link_libraries(test_perm PRIVATE cutl)
add_test(NAME permutations COMMAND test_perm)

add_executable(test_comb ${CUTL_TEST_COMMON_SOURCES} test_combinations.c)
target_link_libraries(test_comb PRIVATE cutl)
add_test(NAME combinations COMMAND test_comb)