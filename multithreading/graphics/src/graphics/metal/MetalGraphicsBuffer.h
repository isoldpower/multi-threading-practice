#pragma once

#include <Metal/Metal.hpp>

#include <graphics/GraphicsBuffer.h>

#include <cstddef>
#include <span>
#include <utility>


namespace multithreading::graphics {

    class MetalBufferHandle {
    public:
        explicit MetalBufferHandle(NS::SharedPtr<MTL::Buffer> buffer)
            : native_buffer(std::move(buffer)) {
        }

        [[nodiscard]] MTL::Buffer* native() const {
            return native_buffer.get();
        }
    private:
        NS::SharedPtr<MTL::Buffer> native_buffer;
    };

    class MetalGraphicsSharedBuffer final : public GraphicsSharedBuffer, public MetalBufferHandle {
    public:
        explicit MetalGraphicsSharedBuffer(NS::SharedPtr<MTL::Buffer> buffer)
            : MetalBufferHandle(std::move(buffer)) {
        }

        [[nodiscard]] std::size_t size_bytes() const override {
            return native()->length();
        }

        [[nodiscard]] std::span<std::byte> host_view() override {
            return {
                static_cast<std::byte*>(native()->contents()),
                native()->length()
            };
        }
    };

    class MetalGraphicsBuffer final : public GraphicsBuffer, public MetalBufferHandle {
    public:
        explicit MetalGraphicsBuffer(NS::SharedPtr<MTL::Buffer> buffer)
            : MetalBufferHandle(std::move(buffer)) {
        }

        [[nodiscard]] std::size_t size_bytes() const override {
            return native()->length();
        }
    };
} // namespace multithreading::graphics
