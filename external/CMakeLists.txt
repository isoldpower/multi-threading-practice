include(FetchContent)

# Include libraries on testing
if (BUILD_TESTING)
    FetchContent_Declare(googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG b514bdc898e2951020cbdca1304b75f5950d1f59)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif ()

# Pull Metal-CPP contents
if (CMAKE_Metal_COMPILER)
    FetchContent_Declare(metal_cpp
            URL https://developer.apple.com/metal/cpp/files/metal-cpp_26.zip
            URL_HASH SHA256=4df3c078b9aadcb516212e9cb03004cbc5ce9a3e9c068fa3144d021db585a3a4)
    FetchContent_MakeAvailable(metal_cpp)

    add_library(metal_cpp INTERFACE)
    target_include_directories(metal_cpp SYSTEM INTERFACE "${metal_cpp_SOURCE_DIR}")
    target_link_libraries(metal_cpp INTERFACE "-framework Foundation" "-framework Metal" "-framework QuartzCore")
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