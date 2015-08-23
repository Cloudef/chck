add_library(chck_string string.c)
install_libraries(chck_string)
install_headers(string.h)

if (CHCK_BUILD_TESTS)
   add_executable(string_test test.c)
   target_link_libraries(string_test PRIVATE chck_string)
   add_test_ex(string_test)
endif ()
