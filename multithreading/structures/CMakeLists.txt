add_library(structures
        src/placeholder.cpp)
include_directories(include)
target_link_libraries(structures PUBLIC cxx_setup utilities)

if (BUILD_TESTING)
    add_executable(structures_tests
            tests/bounded_queue/BoundedQueue_test.cpp
            tests/linked_list/LinkedList_test.cpp
    )
    target_link_libraries(structures_tests PRIVATE GTest::gtest_main structures utilities)
    include(GoogleTest)
    gtest_discover_tests(structures_tests)
endif()

