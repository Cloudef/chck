add_library(chck_pool pool.c)
install_libraries(chck_pool)
install_headers(pool.h)

if (CHCK_BUILD_TESTS)
   add_executable(pool_test test.c)
   target_link_libraries(pool_test PRIVATE chck_pool)
   add_test_ex(pool_test)
endif ()
