# Header-only: the sorts are templates, so nothing compiles until a consumer picks a type.
add_library(parallel_computing INTERFACE)

# GPU sort kernels; consumers get the library path and must depend on gpu_sort_shaders
if (CMAKE_Metal_COMPILER)
    add_metal_shader_library(gpu_sort_shaders STANDARD metal3.0 shaders/gpu_sort.metal)
    target_compile_definitions(parallel_computing
            INTERFACE MT_GPU_SORT_LIBRARY="$<TARGET_FILE:gpu_sort_shaders>")
endif ()

# Tests setup
if (BUILD_TESTING)
    add_executable(parallel_computing_tests
            tests/merge_sort/MergeSort_test.cpp)

    if (TARGET gpu_sort_shaders)
        target_sources(parallel_computing_tests PRIVATE tests/merge_sort/GPUSort_test.cpp)
        add_dependencies(parallel_computing_tests gpu_sort_shaders)
    endif ()

    target_link_libraries(parallel_computing_tests PRIVATE GTest::gtest_main parallel_computing)
    include(GoogleTest)
    gtest_discover_tests(parallel_computing_tests)
endif()

# Expose settings
target_include_directories(parallel_computing
        INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include
)
attach_headers(parallel_computing)

# Link dependencies
target_link_libraries(parallel_computing INTERFACE graphics utilities cxx_setup)
