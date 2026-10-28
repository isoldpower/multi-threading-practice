add_executable(sorting_benchmark sorting_benchmark.cpp)
target_link_libraries(sorting_benchmark
        PRIVATE multithreading_runtime multithreading_computing_scenarios cxx_setup)
