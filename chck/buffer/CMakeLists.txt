set(incs "")
set(libs "")

find_package(ZLIB)
if (ZLIB_FOUND)
   list(APPEND incs ${ZLIB_INCLUDE_DIRS})
   list(APPEND libs ${ZLIB_LIBRARIES})
   add_definitions(-DHAS_ZLIB=1)
endif (ZLIB_FOUND)

include_directories(${incs})
add_executable(buffer_test test.c buffer.c)
target_link_libraries(buffer_test ${libs})
add_test_ex(buffer_test)
