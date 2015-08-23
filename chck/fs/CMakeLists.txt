add_library(chck_fs fs.c)
install_libraries(chck_fs)
install_headers(fs.h)

if (CHCK_BUILD_TESTS)
   add_executable(fs_test test.c)
   target_link_libraries(fs_test PRIVATE chck_fs)
   add_test_ex(fs_test)
endif ()
