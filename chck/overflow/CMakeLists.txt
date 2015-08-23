install_headers(overflow.h)

if (CHCK_BUILD_TESTS)
   add_executable(overflow_test test.c)
   add_test_ex(overflow_test)
endif ()
