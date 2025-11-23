include(FetchContent)
include(CTest)

# Include libraries on testing
if (BUILD_TESTING)
    FetchContent_Declare(googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG b514bdc898e2951020cbdca1304b75f5950d1f59)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif ()

# Disable clang-tidy for external dependencies
if(TARGET gtest)
    set_target_properties(gtest PROPERTIES CXX_CLANG_TIDY "")
endif()
if(TARGET gtest_main)
    set_target_properties(gtest_main PROPERTIES CXX_CLANG_TIDY "")
endif()
if(TARGET gmock)
    set_target_properties(gmock PROPERTIES CXX_CLANG_TIDY "")
endif()
if(TARGET gmock_main)
    set_target_properties(gmock_main PROPERTIES CXX_CLANG_TIDY "")
endif()