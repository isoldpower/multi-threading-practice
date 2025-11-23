add_library(tools
        tools.cpp tools.h
        named_tools.cpp named_tools.h)

if (BUILD_TESTING)
    add_executable(tests tests/main.cpp)
    target_link_libraries(tests PRIVATE GTest::gtest_main)
    include(GoogleTest)
    gtest_discover_tests(tests)
endif()

target_link_libraries(tools PUBLIC cxx_setup)