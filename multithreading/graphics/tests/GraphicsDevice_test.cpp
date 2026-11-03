#include "./GraphicsTestSupport.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <numeric>
#include <stdexcept>
#include <vector>

using multithreading::graphics::testing::GraphicsTest;
using multithreading::graphics::testing::make_buffer;
using multithreading::graphics::testing::read_values;

using GraphicsDeviceTest = GraphicsTest;

TEST_F(GraphicsDeviceTest, SharedBufferHasRequestedSize) {
    const auto buffer = device->allocate_shared(256);

    EXPECT_EQ(buffer->size_bytes(), 256U);
    EXPECT_EQ(buffer->host_view().size(), 256U);
}

TEST_F(GraphicsDeviceTest, SharedBufferKeepsHostWrites) {
    std::vector<std::uint32_t> values(1000);
    std::iota(values.begin(), values.end(), 0U);

    const auto buffer = make_buffer(*device, values);

    EXPECT_EQ(read_values(*buffer), values);
}

TEST_F(GraphicsDeviceTest, PrivateBufferHasRequestedSize) {
    const auto buffer = device->allocate_private(512);

    EXPECT_EQ(buffer->size_bytes(), 512U);
}

TEST_F(GraphicsDeviceTest, LoadedKernelReportsNameAndGroupLimit) {
    const auto kernel = device->load_kernel("add_constant");

    EXPECT_EQ(kernel->name(), "add_constant");
    EXPECT_GT(kernel->max_threads_per_group(), 0U);
}

TEST_F(GraphicsDeviceTest, UnknownKernelThrows) {
    EXPECT_THROW(static_cast<void>(device->load_kernel("no_such_kernel")), std::runtime_error);
}
