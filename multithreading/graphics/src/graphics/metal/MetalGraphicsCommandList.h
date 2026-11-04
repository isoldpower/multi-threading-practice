#pragma once

#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>

#include <graphics/GraphicsCommandList.h>

#include "./MetalErrors.h"
#include "./MetalGraphicsBuffer.h"
#include "./MetalGraphicsKernel.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <utility>


namespace multithreading::graphics {

    enum class State : std::uint8_t {
        RECORDING,
        SUBMITTED,
        COMPLETED,
    };

    class MetalGraphicsCommandList final : public GraphicsCommandList {
    public:
        MetalGraphicsCommandList(
            NS::SharedPtr<MTL::CommandBuffer> command_buffer,
            NS::SharedPtr<MTL::ComputeCommandEncoder> command_encoder
        )
            : command_buffer(std::move(command_buffer))
            , command_encoder(std::move(command_encoder)) {
        }

        ~MetalGraphicsCommandList() override {
            if (state == State::RECORDING) {
                command_encoder->endEncoding();
            }
        }

        MetalGraphicsCommandList(const MetalGraphicsCommandList&) = delete;
        MetalGraphicsCommandList& operator=(const MetalGraphicsCommandList&) = delete;
        MetalGraphicsCommandList(MetalGraphicsCommandList&&) = delete;
        MetalGraphicsCommandList& operator=(MetalGraphicsCommandList&&) = delete;

        void dispatch_work(
            const GraphicsKernel& kernel,
            const std::span<GraphicsBuffer* const> buffers,
            const std::span<const std::byte> constants,
            const GridSize threads
        ) override {
            const GridSize group{
                .width = std::max<std::size_t>(1, std::min(kernel.max_threads_per_group(), threads.width)),
                .height = 1,
                .depth = 1,
            };

            this->dispatch_work(kernel, buffers, constants, threads, group);
        }

        void dispatch_work(
            const GraphicsKernel& kernel,
            const std::span<GraphicsBuffer* const> buffers,
            const std::span<const std::byte> constants,
            const GridSize threads,
            const GridSize group
        ) override {
            require_state(
                State::RECORDING,
                "GPU work already submitted via 'submit()'. "
                "Can't make adjustments after the submission.");

            const std::size_t group_threads = group.width * group.height * group.depth;
            if (group_threads == 0 || group_threads > kernel.max_threads_per_group()) {
                throw std::invalid_argument(
                    "Threadgroup size is empty or exceeds the kernel's maximum threads per group");
            }

            if (threads.width != 0 && threads.height != 0 && threads.depth != 0) {
                const auto& metal_kernel = as_backend<MetalGraphicsKernel>(kernel);
                command_encoder->setComputePipelineState(metal_kernel.native());
                this->set_pipeline_buffers(buffers);
                this->set_pipeline_constants(buffers, constants);

                command_encoder->dispatchThreads(
                    MTL::Size(threads.width, threads.height, threads.depth),
                    MTL::Size(group.width, group.height, group.depth));
            }
        }

        void submit_list() override {
            require_state(State::RECORDING, "submit_list() called twice");

            command_encoder->endEncoding();
            command_buffer->commit();
            state = State::SUBMITTED;
        }

        void wait_completed() override {
            if (state == State::COMPLETED) {
                return;
            }

            require_state(State::SUBMITTED, "wait_completed() before submit()");
            command_buffer->waitUntilCompleted();
            state = State::COMPLETED;

            if (command_buffer->status() == MTL::CommandBufferStatusError) {
                throw std::runtime_error(
                    describe_metal_error(command_buffer->error(), "GPU command execution failed"));
            }
        }
    private:
        NS::SharedPtr<MTL::CommandBuffer> command_buffer;
        NS::SharedPtr<MTL::ComputeCommandEncoder> command_encoder;
        State state{State::RECORDING};

        void require_state(const State expected, const char* misuse) const {
            if (state != expected) {
                throw std::logic_error(misuse);
            }
        }

        void set_pipeline_buffers(const std::span<GraphicsBuffer* const> buffers) const {
            NS::UInteger buffer_index = 0;
            for (const GraphicsBuffer* buffer : buffers) {
                if (buffer == nullptr) {
                    throw std::invalid_argument("Null buffer passed to dispatch_work()");
                }

                const auto& metal_buffer = as_backend<MetalBufferHandle>(*buffer);
                command_encoder->setBuffer(metal_buffer.native(), 0, buffer_index++);
            }
        }

        void set_pipeline_constants(
            const std::span<GraphicsBuffer* const> buffers,
            const std::span<const std::byte> constants
        ) const {
            if (!constants.empty()) {
                command_encoder->setBytes(constants.data(), constants.size(), buffers.size());
            }
        }
    };
} // namespace multithreading::graphics
