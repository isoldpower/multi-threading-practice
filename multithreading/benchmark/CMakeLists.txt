add_library(benchmark
        src/SpeedMeasurement.cpp
        src/mcmp/MCMPBenchmarkRunner.cpp)

if (BUILD_TESTING)
    add_executable(benchmark_tests tests/main.cpp)
    target_link_libraries(benchmark_tests PRIVATE GTest::gtest_main)
    include(GoogleTest)
    gtest_discover_tests(benchmark_tests)
endif()

include_directories(include)
target_link_libraries(benchmark PUBLIC cxx_setup)