# Compile files
add_library(benchmark
        # Runners
        src/benchmark/mcmp/MCMPBenchmarkRunner.cpp
        # Measurements
        src/benchmark/SpeedMeasurement.cpp
        src/benchmark/PeakMemoryMeasurement.cpp
        src/benchmark/AverageMemoryMeasurement.cpp
)

# Tests setup
if (BUILD_TESTING)
    add_executable(benchmark_tests tests/main.cpp)
    target_link_libraries(benchmark_tests PRIVATE GTest::gtest_main benchmark)
    include(GoogleTest)
    gtest_discover_tests(benchmark_tests)
endif()

# Expose settings
target_include_directories(benchmark
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link dependencies
target_link_libraries(benchmark PUBLIC utilities cxx_setup)
