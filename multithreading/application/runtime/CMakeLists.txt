# Header-only, so nothing to compile; consumers pick the headers up through the interface.
add_library(multithreading_runtime INTERFACE)

# Tests setup
if (BUILD_TESTING)
    add_executable(multithreading_runtime_tests
            tests/Application_test.cpp)
    target_link_libraries(multithreading_runtime_tests PRIVATE GTest::gtest_main multithreading_runtime utilities)
    include(GoogleTest)
    gtest_discover_tests(multithreading_runtime_tests)
endif()

# Expose settings
target_include_directories(multithreading_runtime
        INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link dependencies
target_link_libraries(multithreading_runtime INTERFACE cxx_setup)
