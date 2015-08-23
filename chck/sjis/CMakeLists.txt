add_library(chck_sjis sjis.c)
install_libraries(chck_sjis)
install_headers(sjis.h)

if (CHCK_BUILD_TESTS)
   add_executable(sjis_test test.c)
   target_link_libraries(sjis_test PRIVATE chck_sjis)
   add_test_ex(sjis_test)
endif ()
