# - Find Valgrind
#
# VALGRIND_FOUND
# VALGRIND_INCLUDE_DIRS

find_path(VALGRIND_INCLUDE_DIR NAMES valgrind/valgrind.h valgrind/helgrind.h valgrind/memcheck.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(valgrind DEFAULT_MSG VALGRIND_INCLUDE_DIR)

if (VALGRIND_FOUND)
   set(VALGRIND_INCLUDE_DIRS ${VALGRIND_INCLUDE_DIR})
endif ()

mark_as_advanced(VALGRIND_INCLUDE_DIR)


