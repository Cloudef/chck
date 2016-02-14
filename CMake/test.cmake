# Wraps add_test
# Uses CTEST_EXEC_WITH variable which you can set to for example valgrind to run tests with valgrind.
# Also uses CTEST_OUTPUT_DIRECTORY to set global output directory

function(add_test_ex target)
   add_test(NAME ${target} COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} ${CTEST_EXEC_WITH} $<TARGET_FILE:${target}>)
   set_target_properties(${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CTEST_OUTPUT_DIRECTORY}")
endfunction()
