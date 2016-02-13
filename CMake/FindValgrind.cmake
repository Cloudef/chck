# - Find Valgrind
#
# VALGRIND_FOUND
# VALGRIND_INCLUDE_DIRS

find_path(VALGRIND_INCLUDE_DIRS NAMES valgrind/valgrind.h valgrind/helgrind.h valgrind/memcheck.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(valgrind DEFAULT_MSG VALGRIND_INCLUDE_DIRS)
mark_as_advanced(VALGRIND_INCLUDE_DIRS)
