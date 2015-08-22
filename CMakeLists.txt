CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(chck C)
set(CHCK_NAME "Chck")
set(CHCK_DESCRIPTION "Collection of C utilities taken and cleaned up from my other projects")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${chck_SOURCE_DIR}/CMake)
include(CTest)
include(test)

set(CTEST_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/test)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CTEST_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)

if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
   set(ldflags -Wl,-O1,--no-undefined,--sort-common,--as-needed,-z,relro,-z,now)
   set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${ldflags}")
   set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${ldflags}")
   set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${ldflags}")
   add_definitions(
      -Wall -Wextra -Wno-variadic-macros -Wno-long-long -Wformat=2 -Winit-self -Wfloat-equal
      -Wcast-align -Wpointer-arith -Wmissing-prototypes -Wsuggest-attribute=pure -Wsuggest-attribute=const)
   add_definitions(-fstack-protector-strong -fstack-check -pie -flto)
endif ()

if (UNIX AND CMAKE_COMPILER_IS_GNUCC)
   set(CMAKE_POSITION_INDEPENDENT_CODE ON)
   if (${CMAKE_VERSION} VERSION_LESS 2.8.9)
      add_definitions(-fPIC)
   endif ()
endif ()

# We use C++11 for C++ stuff
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# And C99 for C stuff
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -D_DEFAULT_SOURCE")

# <chck/**/foobar.h> include prefix is allowed
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Code
add_subdirectory(chck)

# Documentation
add_subdirectory(doxygen)
