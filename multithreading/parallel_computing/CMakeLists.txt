# Header-only: the sorts are templates, so nothing compiles until a consumer picks a type.
add_library(parallel_computing INTERFACE)

# Tests setup
if (BUILD_TESTING)
    add_executable(parallel_computing_tests
            tests/merge_sort/MergeSort_test.cpp)
    target_link_libraries(parallel_computing_tests PRIVATE GTest::gtest_main parallel_computing)
    include(GoogleTest)
    gtest_discover_tests(parallel_computing_tests)
endif()

# Expose settings
target_include_directories(parallel_computing
        INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include
)
attach_headers(parallel_computing)

# Link dependencies
target_link_libraries(parallel_computing INTERFACE cxx_setup)
