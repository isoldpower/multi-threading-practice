cmake_minimum_required(VERSION 3.16..3.24)
project (cpp_template
        VERSION 0.1.0
        DESCRIPTION "C++ template project structure"
        LANGUAGES CXX)

if (NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE Release CACHE STRING "Type of the build" FORCE)
endif()
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

# Libraries list
add_library(cxx_setup INTERFACE cpp_template/tools)

# General compiler options
if (MSVC)
    target_compile_options(cxx_setup INTERFACE /W4 /WX /permissive-)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(cxx_setup INTERFACE -Wall -Wextra -Wpedantic)
endif()

target_compile_features(cxx_setup
        INTERFACE cxx_std_17)
target_include_directories(cxx_setup
        INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# Subdirectories
add_subdirectory(external)
add_subdirectory(${PROJECT_NAME})
add_subdirectory(executables)
