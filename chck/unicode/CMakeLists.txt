add_library(chck_unicode unicode.c)
install_libraries(chck_unicode)
install_headers(unicode.h)

if (CHCK_BUILD_TESTS)
   add_executable(unicode_test test.c)
   target_link_libraries(unicode_test PRIVATE chck_unicode)
   add_test_ex(unicode_test)
endif ()
