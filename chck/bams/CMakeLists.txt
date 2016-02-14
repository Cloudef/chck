install_headers(bams.h)

if (CHCK_BUILD_TESTS)
   find_package(Math REQUIRED)
   add_executable(bams_test test.c)
   target_link_libraries(bams_test PRIVATE ${MATH_LIBRARY})
   add_test_ex(bams_test)
endif ()
