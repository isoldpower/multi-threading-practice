#pragma once

#include <cstddef>
#include <span>

namespace multithreading::graphics {

    class GraphicsBuffer {
    public:
        GraphicsBuffer() = default;
        virtual ~GraphicsBuffer() = default;

        GraphicsBuffer(const GraphicsBuffer&) = delete;
        GraphicsBuffer& operator=(const GraphicsBuffer&) = delete;
        GraphicsBuffer(GraphicsBuffer&&) = delete;
        GraphicsBuffer& operator=(GraphicsBuffer&&) = delete;

        [[nodiscard]] virtual std::size_t size_bytes() const = 0;
    };

    class ISharedBuffer {
    public:
        ISharedBuffer() = default;
        virtual ~ISharedBuffer() = default;

        ISharedBuffer(const ISharedBuffer&) = delete;
        ISharedBuffer& operator=(const ISharedBuffer&) = delete;
        ISharedBuffer(ISharedBuffer&&) = delete;
        ISharedBuffer& operator=(ISharedBuffer&&) = delete;

        [[nodiscard]] virtual std::span<std::byte> host_view() = 0;
    };

    class GraphicsSharedBuffer : public GraphicsBuffer, public ISharedBuffer {
    public:
        GraphicsSharedBuffer() = default;
    };
} // namespace multithreading::graphics
