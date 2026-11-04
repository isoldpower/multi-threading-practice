#pragma once

#include <string>


namespace multithreading::graphics {

    class GraphicsKernel {
    public:
        GraphicsKernel() = default;
        virtual ~GraphicsKernel() = default;

        GraphicsKernel(const GraphicsKernel&) = delete;
        GraphicsKernel& operator=(const GraphicsKernel&) = delete;
        GraphicsKernel(GraphicsKernel&&) = delete;
        GraphicsKernel& operator=(GraphicsKernel&&) = delete;

        [[nodiscard]] virtual const std::string& name() const = 0;
        [[nodiscard]] virtual std::size_t max_threads_per_group() const = 0;
    };
} // namespace multithreading::graphics