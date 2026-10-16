add_library(parallel_computing
        src/placeholder.cpp)

if (BUILD_TESTING)
    add_executable(parallel_computing_tests tests/main.cpp)
    target_link_libraries(parallel_computing_tests PRIVATE GTest::gtest_main)
    include(GoogleTest)
    gtest_discover_tests(parallel_computing_tests)
endif()

include_directories(include)
target_link_libraries(parallel_computing PUBLIC cxx_setup utilities)