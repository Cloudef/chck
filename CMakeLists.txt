CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(chck)
INCLUDE(CTest)
SET(GLHCK_NAME "Chck")
SET(GLHCK_DESCRIPTION "Collection of C utilities taken and cleaned up from my other projects")
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${chck_SOURCE_DIR}/CMake/modules)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY test)
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY lib)
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY lib)

# Sources
SET(SOURCE buffer sjis fs)
SET(LIBSRC "") # sources to be compiled
SET(LIBINC "") # include directories
SET(LIBLIB "") # libraries to be linked
SET(LIBDEF "") # compile defines
SET(LIBPUB "") # public headers

# Optional features
FIND_PACKAGE(ZLIB)
IF (ZLIB_FOUND)
   LIST(APPEND buffer_INC ${ZLIB_INCLUDE_DIRS})
   LIST(APPEND buffer_DEF -DHAS_ZLIB=1)
   LIST(APPEND buffer_LIB ${ZLIB_LIBRARIES})
ENDIF (ZLIB_FOUND)

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

# -fPIC when we need
SET(FPIC "-fPIC")
IF (WIN32)
   SET(FPIC "")
ENDIF ()

# Collect data for static lib and create tests
FOREACH (src ${SOURCE})
   LIST(APPEND LIBSRC ${src}/${src}.c)
   LIST(APPEND LIBINC ${${src}_INC})
   LIST(APPEND LIBLIB ${${src}_LIB})
   LIST(APPEND LIBDEF ${${src}_DEF})
   LIST(APPEND LIBPUB ${src}/${src}.h)

   INCLUDE_DIRECTORIES(${${src}_INC})
   ADD_DEFINITIONS(${FPIC} ${${src}_DEF})
   ADD_EXECUTABLE(${src}_test ${src}/test.c ${src}/${src}.c)
   TARGET_LINK_LIBRARIES(${src}_test ${${src}_LIB})

   IF (WIN32)
      ADD_TEST(${src}_test test/${src}_test.exe)
   ELSE ()
      ADD_TEST(${src}_test test/${src}_test)
   ENDIF ()
ENDFOREACH (src)

# Compile static lib
INCLUDE_DIRECTORIES(${LIBINC})
ADD_DEFINITIONS(${FPIC} ${LIBDEF})
ADD_LIBRARY(chck ${LIBSRC} ${LIBLIB})
TARGET_LINK_LIBRARIES(chck)

# vim: set ts=8 sw=3 tw=0
