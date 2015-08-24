CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
PROJECT(chck VERSION 0.0.1 LANGUAGES C)
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMake")

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

if (MINGW)
   set(BUILD_SHARED_LIBS OFF)
endif ()

set(CTEST_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CTEST_OUTPUT_DIRECTORY}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")

# Compiler options
if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
   include(GCCCompatibleCompilerOptions)
   add_compiler_warnings(-Wall -Wextra -Wno-variadic-macros -Wno-long-long -Wformat=2 -Winit-self -Wfloat-equal -Wcast-align -Wpointer-arith -Wmissing-prototypes -Wsuggest-attribute=pure -Wsuggest-attribute=const)
   if (NOT MINGW)
      create_custom_linker_flags(Upstream -O1 --sort-common --as-needed -z,relro -z,now)
   else ()
      create_custom_linker_flags(Upstream -O1 --sort-common --as-needed -static)
   endif ()
   check_c_compiler_flag(-fstack-protector-strong ok)
   if (ok)
      create_custom_compiler_flags(Upstream -g -O2 -fstack-protector-strong --param ssp-buffer-size=4 -fstack-check -flto)
   else ()
      create_custom_compiler_flags(Upstream -g -O2 -fstack-protector-all --param ssp-buffer-size=4 -fstack-check -flto)
   endif ()
endif ()

# -std=c99 -fpic -fpie -D_DEFAULT_SOURCE
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_definitions(-D_DEFAULT_SOURCE)

# <chck/**/foobar.h> include prefix is allowed
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Code
add_subdirectory(chck)

# Documentation
add_subdirectory(doxygen)

# Phony for add_subproject
add_custom_target(chck)

set(CHCK_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}" CACHE STRING "Include directories of chck" FORCE)
set(CHCK_LIBRARIES ${CHCK_COMPONENTS} CACHE STRING "Libraries needed for chck" FORCE)
mark_as_advanced(CHCK_LIBRARIES CHCK_INCLUDE_DIRS)

if ("${CMAKE_PROJECT_NAME}" STREQUAL "${PROJECT_NAME}")
   feature_summary(WHAT ALL)
endif ()
