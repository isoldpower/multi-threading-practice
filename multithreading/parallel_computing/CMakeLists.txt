# Compile files
add_library(parallel_computing
        # Merge sort
        src/parallel_computing/merge_sort/SyncMergeSort.cpp
        src/parallel_computing/merge_sort/ParallelMergeSort.cpp
)

# Tests setup
if (BUILD_TESTING)
    add_executable(parallel_computing_tests tests/main.cpp)
    target_link_libraries(parallel_computing_tests PRIVATE GTest::gtest_main parallel_computing)
    include(GoogleTest)
    gtest_discover_tests(parallel_computing_tests)
endif()

# Expose settings
target_include_directories(parallel_computing
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link dependencies
target_link_libraries(parallel_computing PUBLIC utilities cxx_setup)
