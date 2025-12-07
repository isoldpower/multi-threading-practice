add_library(structures
        src/placeholder.cpp)

if (BUILD_TESTING)
    add_executable(structures_tests tests/main.cpp)
    target_link_libraries(structures_tests PRIVATE GTest::gtest_main)
    include(GoogleTest)
    gtest_discover_tests(structures_tests)
endif()

include_directories(include)
target_link_libraries(structures PUBLIC cxx_setup)