add_library(utilities
        src/placeholder.cpp)

if (BUILD_TESTING)
    add_executable(tests tests/main.cpp)
    target_link_libraries(tests PRIVATE GTest::gtest_main)
    include(GoogleTest)
    gtest_discover_tests(tests)
endif()

include_directories(include)
target_link_libraries(utilities PUBLIC cxx_setup)