#include "./GraphicsTestSupport.h"

#include <gtest/gtest.h>
#include <graphics/GraphicsFactory.h>

#include <stdexcept>

using multithreading::graphics::GraphicsFactory;
using multithreading::graphics::GraphicsOptions;
using multithreading::graphics::testing::test_device;

TEST(GraphicsFactoryTest, BuildsNoDeviceWithoutBackend) {
#ifdef MT_GRAPHICS_TEST_LIBRARY
    GTEST_SKIP() << "Built with a GPU backend";
#else
    EXPECT_EQ(GraphicsFactory::build_graphics(GraphicsOptions{}), nullptr);
#endif
}

TEST(GraphicsFactoryTest, BuildsDeviceForValidLibrary) {
    if (test_device() == nullptr) {
        GTEST_SKIP() << "No supported GPU on this machine";
    }

    EXPECT_NE(test_device(), nullptr);
}

TEST(GraphicsFactoryTest, MissingShaderLibraryThrows) {
    if (test_device() == nullptr) {
        GTEST_SKIP() << "No supported GPU on this machine";
    }

    EXPECT_THROW(
        static_cast<void>(GraphicsFactory::build_graphics(
            GraphicsOptions{.shader_library_path = "/nonexistent/missing.metallib"})),
        std::runtime_error);
}
