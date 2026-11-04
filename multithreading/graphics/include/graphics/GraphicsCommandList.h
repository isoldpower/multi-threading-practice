#pragma once

#include <cstddef>
#include <span>
#include <stdexcept>
#include <type_traits>

#include "./GraphicsBuffer.h"
#include "./GraphicsKernel.h"


namespace multithreading::graphics {

    struct GridSize {  // NOLINT(altera-struct-pack-align)
        std::size_t width = 1;
        std::size_t height = 1;
        std::size_t depth = 1;
    };

    class GraphicsCommandList {
    public:
        GraphicsCommandList() = default;
        virtual ~GraphicsCommandList() = default;

        GraphicsCommandList(const GraphicsCommandList&) = delete;
        GraphicsCommandList& operator=(const GraphicsCommandList&) = delete;
        GraphicsCommandList(GraphicsCommandList&&) = delete;
        GraphicsCommandList& operator=(GraphicsCommandList&&) = delete;

        virtual void dispatch_work(
            const GraphicsKernel& kernel,
            std::span<GraphicsBuffer* const> buffers,
            std::span<const std::byte> constants,
            GridSize threads
        ) = 0;

        virtual void dispatch_work(
            const GraphicsKernel& kernel,
            std::span<GraphicsBuffer* const> buffers,
            std::span<const std::byte> constants,
            GridSize threads,
            GridSize group
        ) = 0;

        virtual void submit_list() = 0;
        virtual void wait_completed() = 0;
    protected:
        template <typename TBackend, typename TResource>
            requires std::is_polymorphic_v<TResource>
        static const TBackend& as_backend(const TResource& resource) {
            const auto* backend_resource = dynamic_cast<const TBackend*>(&resource);
            if (backend_resource == nullptr) {
                throw std::invalid_argument("Graphics resource was created by a different backend");
            }

            return *backend_resource;
        }
    };
} // namespace multithreading::graphics
