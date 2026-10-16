add_executable(unbounded_queue_benchmark unbounded_queue_benchmark.cpp)
target_link_libraries(unbounded_queue_benchmark
        PRIVATE multithreading_runtime multithreading_structures_scenarios cxx_setup)

add_executable(bounded_queue_benchmark bounded_queue_benchmark.cpp)
target_link_libraries(bounded_queue_benchmark
        PRIVATE multithreading_runtime multithreading_structures_scenarios cxx_setup)

add_executable(linked_list_benchmark linked_list_benchmark.cpp)
target_link_libraries(linked_list_benchmark
        PRIVATE multithreading_runtime multithreading_structures_scenarios cxx_setup)
