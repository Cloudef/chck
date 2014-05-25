CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(chck)
INCLUDE(CTest)
SET(CHCK_NAME "Chck")
SET(CHCK_DESCRIPTION "Collection of C utilities taken and cleaned up from my other projects")
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${chck_SOURCE_DIR}/CMake/modules)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY test)
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY lib)
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY lib)

# Sources
SET(SOURCE buffer sjis fs json pool array lut cdl atlas)
SET(LIBSRC "") # sources to be compiled
SET(LIBINC "") # include directories
SET(LIBLIB "") # libraries to be linked
SET(LIBDEF "") # compile defines
SET(LIBPUB "") # public headers

# C++ interface
SET(json_TEST "jsonpp-test.cpp")

# Optional features
FIND_PACKAGE(ZLIB)
IF (ZLIB_FOUND)
   LIST(APPEND buffer_INC ${ZLIB_INCLUDE_DIRS})
   LIST(APPEND buffer_DEF -DHAS_ZLIB=1)
   LIST(APPEND buffer_LIB ${ZLIB_LIBRARIES})
ENDIF (ZLIB_FOUND)

IF (UNIX)
   LIST(APPEND cdl_LIB dl)
ENDIF (UNIX)

# Warnings
IF (MSVC)
   IF (CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
      STRING(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
   ELSE ()
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
   ENDIF ()
ELSEIF (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
   SET(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -Wall -Wextra -Wno-variadic-macros -Wno-long-long")
   SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-variadic-macros -Wno-long-long")
ENDIF ()

IF (UNIX AND CMAKE_COMPILER_IS_GNUCC)
   SET(CMAKE_POSITION_INDEPENDENT_CODE ON)
   IF (${CMAKE_VERSION} VERSION_LESS 2.8.9)
      ADD_DEFINITIONS(-fPIC)
   ENDIF ()
ENDIF ()

# We use C++11 for C++ stuff
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# Collect data for static lib and create tests
FOREACH (src ${SOURCE})
   LIST(APPEND LIBSRC ${src}/${src}.c)
   LIST(APPEND LIBINC ${${src}_INC})
   LIST(APPEND LIBLIB ${${src}_LIB})
   LIST(APPEND LIBDEF ${${src}_DEF})
   LIST(APPEND LIBPUB ${src}/${src}.h)
   SET(tests "test.c" ${${src}_TEST})

   FOREACH (testsrc ${tests})
      GET_FILENAME_COMPONENT(test ${testsrc} NAME_WE)
      INCLUDE_DIRECTORIES(${${src}_INC})
      ADD_DEFINITIONS(${${src}_DEF})
      ADD_EXECUTABLE(${src}_${test} ${src}/${testsrc} ${src}/${src}.c)
      TARGET_LINK_LIBRARIES(${src}_${test} ${${src}_LIB})

      IF (WIN32)
         ADD_TEST(${src}_${test} ${TEST_PRG} test/${src}_${test}.exe)
      ELSE ()
         ADD_TEST(${src}_${test} ${TEST_PRG} test/${src}_${test})
      ENDIF ()
   ENDFOREACH (testsrc)
ENDFOREACH (src)

# Compile static lib
INCLUDE_DIRECTORIES(${LIBINC})
ADD_DEFINITIONS(${LIBDEF})
ADD_LIBRARY(chck ${LIBSRC})
TARGET_LINK_LIBRARIES(chck ${LIBLIB})

# Documentation
ADD_SUBDIRECTORY(doxygen)

# vim: set ts=8 sw=3 tw=0
