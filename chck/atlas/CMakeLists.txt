add_library(chck_atlas atlas.c)
install_libraries(chck_atlas)
install_headers(atlas.h)

if (CHCK_BUILD_TESTS)
   add_executable(atlas_test test.c)
   target_link_libraries(atlas_test PRIVATE chck_atlas)
   add_test_ex(atlas_test)
endif ()
