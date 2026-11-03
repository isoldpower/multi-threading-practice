#pragma once

#include <cstddef>
#include <memory>
#include <string>

#include "./GraphicsBuffer.h"
#include "./GraphicsCommandList.h"
#include "./GraphicsKernel.h"


namespace multithreading::graphics {

    class GraphicsDevice {
    public:
        GraphicsDevice() = default;
        virtual ~GraphicsDevice() = default;

        GraphicsDevice(const GraphicsDevice&) = delete;
        GraphicsDevice& operator=(const GraphicsDevice&) = delete;
        GraphicsDevice(GraphicsDevice&&) = delete;
        GraphicsDevice& operator=(GraphicsDevice&&) = delete;

        virtual std::unique_ptr<GraphicsSharedBuffer> allocate_shared(std::size_t bytes) = 0;
        virtual std::unique_ptr<GraphicsBuffer> allocate_private(std::size_t bytes) = 0;

        virtual std::unique_ptr<GraphicsKernel> load_kernel(const std::string& kernel_name) = 0;
        virtual std::unique_ptr<GraphicsCommandList> begin_commands() = 0;
    };
} // namespace multithreading::graphics
