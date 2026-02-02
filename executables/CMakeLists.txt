add_executable(unbounded_queue_benchmark
        unbounded_queue_benchmark.cpp
        benchmarks/src/UnboundedQueueBenchmark.cpp
        benchmarks/src/mcmp/UnboundedQueueMCMPBenchmark.cpp)
add_executable(bounded_queue_benchmark
        bounded_queue_benchmark.cpp
        benchmarks/src/BoundedQueueBenchmark.cpp
        benchmarks/src/mcmp/BoundedQueueMCMPBenchmark.cpp)
add_executable(linked_list_benchmark
        linked_list_benchmark.cpp
        benchmarks/src/LinkedListBenchmark.cpp
        benchmarks/src/mcmp/LinkedListMCMPBenchmark.cpp)

target_link_libraries(unbounded_queue_benchmark PRIVATE utilities structures cxx_setup)
target_link_libraries(bounded_queue_benchmark PRIVATE utilities structures cxx_setup)
target_link_libraries(linked_list_benchmark PRIVATE utilities structures cxx_setup)