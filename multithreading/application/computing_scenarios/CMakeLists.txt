# Compile files
add_library(multithreading_computing_scenarios
        # Merge sort
        src/application/computing_scenarios/merge_sort/MergeSortBenchmark.cpp
)

# Expose settings
target_include_directories(multithreading_computing_scenarios
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link dependencies
target_link_libraries(multithreading_computing_scenarios PUBLIC parallel_computing benchmark cxx_setup)
