# Metal-only sample: without the Metal toolchain there is no metal-cpp to build it against
if (CMAKE_Metal_COMPILER)
    add_executable(test_run_metal run_metal_compute.cpp)
    target_link_libraries(test_run_metal PRIVATE metal_cpp cxx_setup)

    add_metal_shader_library(test_shaders STANDARD metal3.0 shaders/add_one.metal)
    target_copy_metal_shader_libraries(test_run_metal test_shaders)
    target_compile_definitions(test_run_metal PRIVATE SHADER_LIBRARY_NAME="$<TARGET_FILE_NAME:test_shaders>")
endif ()
