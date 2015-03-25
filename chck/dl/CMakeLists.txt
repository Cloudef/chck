set(libs "")

if (UNIX)
   list(APPEND libs "dl")
endif ()

add_executable(dl_test test.c dl.c)
target_link_libraries(dl_test ${libs})
add_test_ex(dl_test)
