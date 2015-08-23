CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
PROJECT(chck VERSION 0.0.1 LANGUAGES C)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${chck_SOURCE_DIR}/CMake")

# CPack
set(CPACK_SYSTEM_NAME "${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_GENERATOR "7Z")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/pkg")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Collection of C utilites")
set(CPACK_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION ON)

# Includes
include(GNUInstallDirs)
include(FeatureSummary)
include(CPack)
include(CTest)
include(test)

OPTION(CHCK_BUILD_STATIC "Build chck as static library" OFF)
OPTION(CHCK_BUILD_TESTS "Build chck tests" ON)

add_feature_info(Static CHCK_BUILD_STATIC "Compile as static library")
add_feature_info(Tests CHCK_BUILD_TESTS "Compile tests")

if (NOT CHCK_BUILD_STATIC)
   set(BUILD_SHARED_LIBS ON)
endif ()

set(CTEST_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CTEST_OUTPUT_DIRECTORY}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")

if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
   set(ldflags -Wl,-O1,--no-undefined,--sort-common,--as-needed,-z,relro,-z,now)
   set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${ldflags}")
   set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${ldflags}")
   set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${ldflags}")

   if ("${CMAKE_C_FLAGS}" STREQUAL "")
      add_definitions(-O2)
      add_definitions(-fstack-protector-strong -fstack-check -flto)
   endif ()

   add_definitions(
      -Wall -Wextra -Wno-variadic-macros -Wno-long-long -Wformat=2 -Winit-self -Wfloat-equal
      -Wcast-align -Wpointer-arith -Wmissing-prototypes -Wsuggest-attribute=pure -Wsuggest-attribute=const)
endif ()

# -std=c99 -fpic -fpie -D_DEFAULT_SOURCE
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE OFF)
add_definitions(-D_DEFAULT_SOURCE)

# <chck/**/foobar.h> include prefix is allowed
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Code
add_subdirectory(chck)

# Documentation
add_subdirectory(doxygen)

feature_summary(WHAT ALL)
