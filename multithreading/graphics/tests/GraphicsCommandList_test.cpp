#include "./GraphicsTestSupport.h"

#include <gtest/gtest.h>
#include <graphics/GraphicsBuffer.h>
#include <graphics/GraphicsCommandList.h>
#include <graphics/GraphicsKernel.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

using multithreading::graphics::GraphicsBuffer;
using multithreading::graphics::GraphicsKernel;
using multithreading::graphics::GridSize;
using multithreading::graphics::testing::AddParameters;
using multithreading::graphics::testing::as_constants;
using multithreading::graphics::testing::GraphicsTest;
using multithreading::graphics::testing::make_buffer;
using multithreading::graphics::testing::read_values;
using multithreading::graphics::testing::run;

namespace {
    /** Resources from some other backend: the Metal command list must refuse them. */
    class ForeignBuffer final : public GraphicsBuffer {
    public:
        [[nodiscard]] std::size_t size_bytes() const override {
            return 0;
        }
    };

    class ForeignKernel final : public GraphicsKernel {
    public:
        [[nodiscard]] const std::string& name() const override {
            return kernel_name;
        }

        [[nodiscard]] std::size_t max_threads_per_group() const override {
            return 1024;
        }

    private:
        std::string kernel_name = "foreign";
    };

    class GraphicsCommandListTest : public GraphicsTest {
    protected:
        /** Records an add_constant dispatch over the whole buffer. */
        static void add(
            multithreading::graphics::GraphicsCommandList& commands,
            const GraphicsKernel& kernel,
            GraphicsBuffer& buffer,
            const std::uint32_t addend
        ) {
            const AddParameters parameters{.addend = addend};
            const std::array<GraphicsBuffer*, 1> buffers{&buffer};
            commands.dispatch_work(
                kernel,
                buffers,
                as_constants(parameters),
                GridSize{.width = buffer.size_bytes() / sizeof(std::uint32_t)});
        }
    };
}  // namespace

TEST_F(GraphicsCommandListTest, DispatchAppliesKernelToEveryElement) {
    std::vector<std::uint32_t> values(4096);
    std::iota(values.begin(), values.end(), 0U);
    const auto buffer = make_buffer(*device, values);
    const auto kernel = device->load_kernel("add_constant");

    const auto commands = device->begin_commands();
    add(*commands, *kernel, *buffer, 7);
    run(*commands);

    for (std::uint32_t& value : values) {
        value += 7;
    }
    EXPECT_EQ(read_values(*buffer), values);
}

TEST_F(GraphicsCommandListTest, GridSmallerThanBufferLeavesTailUntouched) {
    const auto buffer = make_buffer(*device, std::vector<std::uint32_t>(1000, 0));
    const auto kernel = device->load_kernel("add_constant");
    const AddParameters parameters{.addend = 1};
    const std::array<GraphicsBuffer*, 1> buffers{buffer.get()};

    // 999 is not a multiple of 64, so the last threadgroup is partial
    const auto commands = device->begin_commands();
    commands->dispatch_work(
        *kernel, buffers, as_constants(parameters), GridSize{.width = 999}, GridSize{.width = 64});
    run(*commands);

    std::vector<std::uint32_t> expected(1000, 1);
    expected.back() = 0;
    EXPECT_EQ(read_values(*buffer), expected);
}

TEST_F(GraphicsCommandListTest, TwoDimensionalGridCoversEveryPosition) {
    constexpr std::uint32_t width = 37;
    constexpr std::uint32_t height = 13;
    constexpr std::size_t count = std::size_t{width} * height;
    const auto buffer = make_buffer(*device, std::vector<std::uint32_t>(count, 0));
    const auto kernel = device->load_kernel("write_grid_position");
    const std::array<GraphicsBuffer*, 1> buffers{buffer.get()};

    const auto commands = device->begin_commands();
    commands->dispatch_work(
        *kernel, buffers, as_constants(width), GridSize{.width = width, .height = height});
    run(*commands);

    std::vector<std::uint32_t> expected;
    for (std::uint32_t row = 0; row < height; ++row) {
        for (std::uint32_t column = 0; column < width; ++column) {
            expected.push_back((row * 1000) + column);
        }
    }
    EXPECT_EQ(read_values(*buffer), expected);
}

TEST_F(GraphicsCommandListTest, ExplicitGroupSizeIsUsed) {
    const auto buffer = make_buffer(*device, std::vector<std::uint32_t>(256, 0));
    const auto kernel = device->load_kernel("write_group_size");
    const std::array<GraphicsBuffer*, 1> buffers{buffer.get()};

    const auto commands = device->begin_commands();
    commands->dispatch_work(*kernel, buffers, {}, GridSize{.width = 256}, GridSize{.width = 64});
    run(*commands);

    EXPECT_EQ(read_values(*buffer), std::vector<std::uint32_t>(256, 64));
}

TEST_F(GraphicsCommandListTest, DefaultGroupShrinksToSmallGrid) {
    const auto buffer = make_buffer(*device, std::vector<std::uint32_t>(10, 0));
    const auto kernel = device->load_kernel("write_group_size");
    const std::array<GraphicsBuffer*, 1> buffers{buffer.get()};

    const auto commands = device->begin_commands();
    commands->dispatch_work(*kernel, buffers, {}, GridSize{.width = 10});
    run(*commands);

    EXPECT_EQ(read_values(*buffer), std::vector<std::uint32_t>(10, 10));
}

TEST_F(GraphicsCommandListTest, LaterDispatchSeesEarlierResultsThroughPrivateBuffer) {
    std::vector<std::uint32_t> values(2048);
    std::iota(values.begin(), values.end(), 100U);
    const auto source = make_buffer(*device, values);
    const auto gpu_only = device->allocate_private(source->size_bytes());
    const auto destination = make_buffer(*device, std::vector<std::uint32_t>(values.size(), 0));
    const auto kernel = device->load_kernel("copy_values");
    const std::array<GraphicsBuffer*, 2> upload{source.get(), gpu_only.get()};
    const std::array<GraphicsBuffer*, 2> download{gpu_only.get(), destination.get()};

    const auto commands = device->begin_commands();
    commands->dispatch_work(*kernel, upload, {}, GridSize{.width = values.size()});
    commands->dispatch_work(*kernel, download, {}, GridSize{.width = values.size()});
    run(*commands);

    EXPECT_EQ(read_values(*destination), values);
}

TEST_F(GraphicsCommandListTest, LaterListSeesEarlierListResults) {
    const auto buffer = make_buffer(*device, std::vector<std::uint32_t>(512, 0));
    const auto kernel = device->load_kernel("add_constant");

    const auto first = device->begin_commands();
    add(*first, *kernel, *buffer, 1);
    run(*first);

    const auto second = device->begin_commands();
    add(*second, *kernel, *buffer, 2);
    run(*second);

    EXPECT_EQ(read_values(*buffer), std::vector<std::uint32_t>(512, 3));
}

TEST_F(GraphicsCommandListTest, EmptyGridIsSkipped) {
    const auto buffer = make_buffer(*device, std::vector<std::uint32_t>(16, 5));
    const auto kernel = device->load_kernel("add_constant");
    const AddParameters parameters{.addend = 1};
    const std::array<GraphicsBuffer*, 1> buffers{buffer.get()};

    const auto commands = device->begin_commands();
    commands->dispatch_work(*kernel, buffers, as_constants(parameters), GridSize{.width = 0});
    run(*commands);

    EXPECT_EQ(read_values(*buffer), std::vector<std::uint32_t>(16, 5));
}

TEST_F(GraphicsCommandListTest, EmptyListSubmits) {
    const auto commands = device->begin_commands();

    EXPECT_NO_THROW(run(*commands));
}

TEST_F(GraphicsCommandListTest, WaitingTwiceIsAllowed) {
    const auto commands = device->begin_commands();
    run(*commands);

    EXPECT_NO_THROW(commands->wait_completed());
}

TEST_F(GraphicsCommandListTest, UnsubmittedListCanBeDropped) {
    const auto buffer = make_buffer(*device, std::vector<std::uint32_t>(64, 0));
    const auto kernel = device->load_kernel("add_constant");
    {
        const auto dropped = device->begin_commands();
        add(*dropped, *kernel, *buffer, 1);
    }

    const auto commands = device->begin_commands();
    add(*commands, *kernel, *buffer, 2);
    run(*commands);

    EXPECT_EQ(read_values(*buffer), std::vector<std::uint32_t>(64, 2));
}

TEST_F(GraphicsCommandListTest, DispatchAfterSubmitThrows) {
    const auto buffer = make_buffer(*device, std::vector<std::uint32_t>(64, 0));
    const auto kernel = device->load_kernel("add_constant");
    const auto commands = device->begin_commands();
    commands->submit_list();

    EXPECT_THROW(add(*commands, *kernel, *buffer, 1), std::logic_error);
    commands->wait_completed();
}

TEST_F(GraphicsCommandListTest, SubmittingTwiceThrows) {
    const auto commands = device->begin_commands();
    commands->submit_list();

    EXPECT_THROW(commands->submit_list(), std::logic_error);
    commands->wait_completed();
}

TEST_F(GraphicsCommandListTest, WaitingBeforeSubmitThrows) {
    const auto commands = device->begin_commands();

    EXPECT_THROW(commands->wait_completed(), std::logic_error);
}

TEST_F(GraphicsCommandListTest, GroupAboveKernelLimitThrows) {
    const auto buffer = make_buffer(*device, std::vector<std::uint32_t>(64, 0));
    const auto kernel = device->load_kernel("write_group_size");
    const std::array<GraphicsBuffer*, 1> buffers{buffer.get()};
    const auto commands = device->begin_commands();

    EXPECT_THROW(
        commands->dispatch_work(
            *kernel,
            buffers,
            {},
            GridSize{.width = 64},
            GridSize{.width = kernel->max_threads_per_group() + 1}),
        std::invalid_argument);
}

TEST_F(GraphicsCommandListTest, EmptyGroupThrows) {
    const auto buffer = make_buffer(*device, std::vector<std::uint32_t>(64, 0));
    const auto kernel = device->load_kernel("write_group_size");
    const std::array<GraphicsBuffer*, 1> buffers{buffer.get()};
    const auto commands = device->begin_commands();

    EXPECT_THROW(
        commands->dispatch_work(
            *kernel, buffers, {}, GridSize{.width = 64}, GridSize{.width = 0}),
        std::invalid_argument);
}

TEST_F(GraphicsCommandListTest, NullBufferThrows) {
    const auto kernel = device->load_kernel("write_group_size");
    const std::array<GraphicsBuffer*, 1> buffers{nullptr};
    const auto commands = device->begin_commands();

    EXPECT_THROW(
        commands->dispatch_work(*kernel, buffers, {}, GridSize{.width = 64}),
        std::invalid_argument);
}

TEST_F(GraphicsCommandListTest, BufferFromOtherBackendThrows) {
    ForeignBuffer foreign;
    const auto kernel = device->load_kernel("write_group_size");
    const std::array<GraphicsBuffer*, 1> buffers{&foreign};
    const auto commands = device->begin_commands();

    EXPECT_THROW(
        commands->dispatch_work(*kernel, buffers, {}, GridSize{.width = 64}),
        std::invalid_argument);
}

TEST_F(GraphicsCommandListTest, KernelFromOtherBackendThrows) {
    const ForeignKernel foreign;
    const auto buffer = make_buffer(*device, std::vector<std::uint32_t>(64, 0));
    const std::array<GraphicsBuffer*, 1> buffers{buffer.get()};
    const auto commands = device->begin_commands();

    EXPECT_THROW(
        commands->dispatch_work(foreign, buffers, {}, GridSize{.width = 64}),
        std::invalid_argument);
}
