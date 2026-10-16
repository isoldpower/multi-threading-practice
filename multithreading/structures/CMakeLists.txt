# Compile files
add_library(structures
        # Header-only library still needs one translation unit to be checked
        src/structures/placeholder.cpp
)

# Tests setup
if (BUILD_TESTING)
    add_executable(structures_tests
            tests/bounded_queue/BoundedQueue_test.cpp
            tests/linked_list/LinkedList_test.cpp
            tests/unbounded_queue/UnboundedQueue_test.cpp)
    target_link_libraries(structures_tests PRIVATE GTest::gtest_main structures)
    include(GoogleTest)
    gtest_discover_tests(structures_tests)
endif()

# Expose settings
target_include_directories(structures
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link dependencies
target_link_libraries(structures PUBLIC utilities cxx_setup)
