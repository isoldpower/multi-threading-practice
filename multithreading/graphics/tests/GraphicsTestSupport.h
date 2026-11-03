#pragma once

#include <gtest/gtest.h>
#include <graphics/GraphicsBuffer.h>
#include <graphics/GraphicsCommandList.h>
#include <graphics/GraphicsDevice.h>
#include <graphics/GraphicsFactory.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <span>
#include <vector>

namespace multithreading::graphics::testing {

    /** Must match AddParameters in shaders/graphics_test.metal */
    struct AddParameters {
        std::uint32_t addend;
    };

    /**
     * Created once for the whole test binary, since loading the shader library is not free.
     * Null when the build has no GPU backend or the machine has no supported GPU.
     */
    inline GraphicsDevice* test_device() {
#ifdef MT_GRAPHICS_TEST_LIBRARY
        static const std::unique_ptr<GraphicsDevice> device = GraphicsFactory::build_graphics(
            GraphicsOptions{.shader_library_path = MT_GRAPHICS_TEST_LIBRARY});
        return device.get();
#else
        return nullptr;
#endif
    }

    inline std::unique_ptr<GraphicsSharedBuffer> make_buffer(
        GraphicsDevice& device,
        const std::vector<std::uint32_t>& values
    ) {
        auto buffer = device.allocate_shared(values.size() * sizeof(std::uint32_t));
        std::memcpy(buffer->host_view().data(), values.data(), buffer->size_bytes());
        return buffer;
    }

    inline std::vector<std::uint32_t> read_values(GraphicsSharedBuffer& buffer) {
        std::vector<std::uint32_t> values(buffer.size_bytes() / sizeof(std::uint32_t));
        std::memcpy(values.data(), buffer.host_view().data(), buffer.size_bytes());
        return values;
    }

    template <typename T>
    std::span<const std::byte> as_constants(const T& value) {
        return std::as_bytes(std::span<const T, 1>(&value, 1));
    }

    inline void run(GraphicsCommandList& commands) {
        commands.submit_list();
        commands.wait_completed();
    }

    class GraphicsTest : public ::testing::Test {
    protected:
        GraphicsDevice* device = nullptr;

        void SetUp() override {
            device = test_device();
            if (device == nullptr) {
                GTEST_SKIP() << "No supported GPU on this machine";
            }
        }
    };
}  // namespace multithreading::graphics::testing
