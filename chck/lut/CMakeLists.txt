add_library(chck_lut lut.c)
install_libraries(chck_lut)
install_headers(lut.h)

if (CHCK_BUILD_TESTS)
   add_executable(lut_test test.c)
   target_link_libraries(lut_test PRIVATE chck_lut)
   add_test_ex(lut_test)
endif ()
