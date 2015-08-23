install_headers(math.h)

if (CHCK_BUILD_TESTS)
   find_package(Math REQUIRED)
   add_executable(math_test test.c)
   target_link_libraries(math_test PRIVATE ${MATH_LIBRARY})
   add_test_ex(math_test)
endif ()
