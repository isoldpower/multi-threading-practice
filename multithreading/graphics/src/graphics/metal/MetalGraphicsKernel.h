#pragma once

#include <graphics/GraphicsKernel.h>

#include <Foundation/NSSharedPtr.hpp>
#include <Metal/MTLComputePipeline.hpp>


namespace multithreading::graphics {

    class MetalGraphicsKernel final : public GraphicsKernel {
    public:
        MetalGraphicsKernel(std::string kernel_name, NS::SharedPtr<MTL::ComputePipelineState> pipeline)
            : kernel_name(std::move(kernel_name))
            , pipeline(std::move(pipeline)) {
        }

        [[nodiscard]] const std::string& name() const override {
            return kernel_name;
        }

        [[nodiscard]] std::size_t max_threads_per_group() const override {
            return pipeline->maxTotalThreadsPerThreadgroup();
        }

        [[nodiscard]] MTL::ComputePipelineState* native() const {
            return pipeline.get();
        }
    private:
        const std::string kernel_name;
        NS::SharedPtr<MTL::ComputePipelineState> pipeline;
    };
} // namespace multithreading::graphics