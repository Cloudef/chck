add_executable(thread_queue_test queue.c test.c)
target_link_libraries(thread_queue_test ${THREAD_LIB})
add_test_ex(thread_queue_test)
