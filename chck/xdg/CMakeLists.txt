# Not supporting other platforms atm
if (UNIX)

add_library(chck_xdg xdg.c)
install_libraries(chck_xdg)
install_headers(xdg.h)

if (CHCK_BUILD_TESTS)
   add_executable(xdg_test test.c)
   target_link_libraries(xdg_test PRIVATE chck_xdg)
   add_test_ex(xdg_test)
endif ()

endif ()
