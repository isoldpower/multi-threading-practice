add_library(utilities
        src/benchmark/mcmp/MCMPBenchmarkRunner.cpp
        include/benchmark/monitor/SpeedMeasurement.h
        include/ConsoleOutput.h)

if (BUILD_TESTING)
    add_executable(utilities_tests tests/main.cpp)
    target_link_libraries(utilities_tests PRIVATE GTest::gtest_main)
    include(GoogleTest)
    gtest_discover_tests(utilities_tests)
endif()

include_directories(include)
target_link_libraries(utilities PUBLIC cxx_setup)