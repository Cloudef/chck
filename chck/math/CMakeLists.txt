add_executable(math_test test.c)
target_link_libraries(math_test m)
add_test_ex(math_test)
