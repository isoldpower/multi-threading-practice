# Compile files
add_library(graphics
        src/graphics/GraphicsFactory.cpp)

if (CMAKE_Metal_COMPILER)
    target_sources(graphics PRIVATE
            src/graphics/metal/MetalErrors.h
            src/graphics/metal/MetalGraphicsBuffer.h
            src/graphics/metal/MetalGraphicsCommandList.h
            src/graphics/metal/MetalGraphicsDevice.h
            src/graphics/metal/MetalGraphicsKernel.h
            src/graphics/metal/MetalImplementation.cpp)
    target_link_libraries(graphics PRIVATE metal_cpp)
    target_compile_definitions(graphics PRIVATE MT_HAS_METAL=1)
elseif (CMAKE_CUDA_COMPILER)
    target_sources(graphics PRIVATE
            src/graphics/cuda/CUDAGraphicsBuffer.h
            src/graphics/cuda/CUDAGraphicsCommandList.h
            src/graphics/cuda/CUDAGraphicsDevice.h
            src/graphics/cuda/CUDAGraphicsKernel.h)
    find_package(CUDAToolkit REQUIRED)
    target_link_libraries(graphics PRIVATE CUDA::cudart)
    target_compile_definitions(graphics PRIVATE MT_HAS_CUDA=1)
endif()

# Tests setup
if (BUILD_TESTING)
    add_executable(graphics_tests
            tests/GraphicsFactory_test.cpp
            tests/GraphicsDevice_test.cpp
            tests/GraphicsCommandList_test.cpp)
    target_link_libraries(graphics_tests PRIVATE GTest::gtest_main graphics)

    # Without a test shader library the GPU tests skip themselves
    if (CMAKE_Metal_COMPILER)
        add_metal_shader_library(graphics_test_shaders STANDARD metal3.0 tests/shaders/graphics_test.metal)
        add_dependencies(graphics_tests graphics_test_shaders)
        target_compile_definitions(graphics_tests
                PRIVATE MT_GRAPHICS_TEST_LIBRARY="$<TARGET_FILE:graphics_test_shaders>")
    endif ()
    include(GoogleTest)
    gtest_discover_tests(graphics_tests)
endif()

# Expose settings
target_include_directories(graphics
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
attach_headers(graphics)

# Link dependencies
target_link_libraries(graphics PUBLIC cxx_setup)
