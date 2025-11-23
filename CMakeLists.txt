cmake_minimum_required(VERSION 3.16..3.24)
project (multithreading
        VERSION 0.1.0
        DESCRIPTION "Set of tasks from the LETU university multi-threading course."
        LANGUAGES CXX)

if (NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE Release CACHE STRING "Type of the build" FORCE)
endif()
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

# Enable Clang-tidy
find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
if(CLANG_TIDY_EXE)
    set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
else()
    message(WARNING "Clang-Tidy EXE not found; Skipping clang-tidy integration...")
endif()

# Libraries list
add_library(cxx_setup INTERFACE multithreading/utilities)

# General compiler options
if (MSVC)
    target_compile_options(cxx_setup INTERFACE /W4 /WX /permissive-)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(cxx_setup INTERFACE -Wall -Wextra -Wpedantic)
endif()

target_compile_features(cxx_setup
        INTERFACE cxx_std_20)
target_include_directories(cxx_setup
        INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# Subdirectories
add_subdirectory(external)
add_subdirectory(${PROJECT_NAME})
add_subdirectory(executables)
