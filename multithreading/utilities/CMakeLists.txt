add_library(utilities
        src/threads/ThreadBarrier.cpp)

if (BUILD_TESTING)
    add_executable(utilities_tests
            tests/performance/AlignedField_test.cpp
            tests/performance/EpochReclamation_test.cpp
            tests/performance/Memory_test.cpp
            tests/tests/AmorphicTest_test.cpp
            tests/threads/CountThreadBarrier_test.cpp
            tests/threads/ThreadBarrier_test.cpp
            tests/threads/RandomGenerator_test.cpp
            tests/RandomEvent_test.cpp
            tests/Application_test.cpp)
    target_link_libraries(utilities_tests PRIVATE GTest::gtest_main utilities)
    include(GoogleTest)
    gtest_discover_tests(utilities_tests)
endif()

include_directories(include)
target_link_libraries(utilities PUBLIC GTest::gtest_main cxx_setup)