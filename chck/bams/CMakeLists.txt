install_headers(bams.h)

if (CHCK_BUILD_TESTS)
   add_executable(bams_test test.c)
   add_test_ex(bams_test)
endif ()
