# Header-only: the structures are templates, so nothing compiles until a consumer picks a type.
add_library(structures INTERFACE)

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
        INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include
)
attach_headers(structures)

# Link dependencies
target_link_libraries(structures INTERFACE utilities cxx_setup)
