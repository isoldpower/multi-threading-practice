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
            tests/placeholder_test.cpp)
    target_link_libraries(graphics_tests PRIVATE GTest::gtest_main graphics)
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
