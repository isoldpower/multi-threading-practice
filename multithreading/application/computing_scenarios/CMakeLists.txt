# Compile files
add_library(multithreading_computing_scenarios
        # Merge sort
        src/application/computing_scenarios/merge_sort/SortConfig.cpp
        src/application/computing_scenarios/merge_sort/SortInputs.cpp
        src/application/computing_scenarios/merge_sort/MergeSortBenchmark.cpp
        src/application/computing_scenarios/merge_sort/SortScenarios.cpp
)

# Expose settings
target_include_directories(multithreading_computing_scenarios
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
attach_headers(multithreading_computing_scenarios)

# Link dependencies
target_link_libraries(multithreading_computing_scenarios
        PUBLIC parallel_computing
        PUBLIC benchmark
        PUBLIC cxx_setup)

# GPU scenarios load the sort shader library at runtime, so it must be built first
if (TARGET gpu_sort_shaders)
    add_dependencies(multithreading_computing_scenarios gpu_sort_shaders)
endif ()
