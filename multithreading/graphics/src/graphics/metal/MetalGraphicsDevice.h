#pragma once

#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>

#include <graphics/GraphicsDevice.h>

#include "./MetalErrors.h"
#include "./MetalGraphicsBuffer.h"
#include "./MetalGraphicsCommandList.h"
#include "./MetalGraphicsKernel.h"

#include <cstddef>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>


namespace multithreading::graphics {

    class MetalGraphicsDevice final : public GraphicsDevice {
    public:
        MetalGraphicsDevice(NS::SharedPtr<MTL::Device> device, const std::filesystem::path& library_path)
            : device_ptr(std::move(device))
            , loaded_library(load_library(device_ptr.get(), library_path))
            , command_queue(create_queue(device_ptr.get())) {
        }

        std::unique_ptr<GraphicsSharedBuffer> allocate_shared(const std::size_t bytes) override {
            auto created_buffer = allocate_buffer(bytes, MTL::ResourceStorageModeShared);
            return std::make_unique<MetalGraphicsSharedBuffer>(std::move(created_buffer));
        }

        std::unique_ptr<GraphicsBuffer> allocate_private(const std::size_t bytes) override {
            auto created_buffer = allocate_buffer(bytes, MTL::ResourceStorageModePrivate);
            return std::make_unique<MetalGraphicsBuffer>(std::move(created_buffer));
        }

        std::unique_ptr<GraphicsKernel> load_kernel(const std::string& kernel_name) override {
            const auto release_pool = NS::TransferPtr(NS::AutoreleasePool::alloc()->init());

            const NS::String* parsed_name = NS::String::string(kernel_name.c_str(), NS::UTF8StringEncoding);
            const auto kernel_function = NS::TransferPtr(loaded_library->newFunction(parsed_name));
            if (!kernel_function) {
                throw std::runtime_error("Kernel '" + kernel_name + "' not found in shader library");
            }

            NS::Error* kernel_error = nullptr;
            auto loaded_pipeline = NS::TransferPtr(
                device_ptr->newComputePipelineState(kernel_function.get(), &kernel_error));
            if (!loaded_pipeline) {
                throw std::runtime_error(
                    describe_metal_error(kernel_error, "Pipeline creation failed for '" + kernel_name + "'"));
            }

            return std::make_unique<MetalGraphicsKernel>(kernel_name, std::move(loaded_pipeline));
        }

        std::unique_ptr<GraphicsCommandList> begin_commands() override {
            const auto release_pool = NS::TransferPtr(NS::AutoreleasePool::alloc()->init());

            auto command_buffer = NS::RetainPtr(command_queue->commandBuffer());
            if (!command_buffer) {
                throw std::runtime_error("Metal command buffer creation failed");
            }
            auto command_encoder = NS::RetainPtr(command_buffer->computeCommandEncoder());
            if (!command_encoder) {
                throw std::runtime_error("Metal compute encoder creation failed");
            }

            return std::make_unique<MetalGraphicsCommandList>(
                std::move(command_buffer), std::move(command_encoder));
        }

    private:
        NS::SharedPtr<MTL::Device> device_ptr;
        NS::SharedPtr<MTL::Library> loaded_library;
        NS::SharedPtr<MTL::CommandQueue> command_queue;

        static NS::SharedPtr<MTL::CommandQueue> create_queue(MTL::Device* device) {
            auto queue = NS::TransferPtr(device->newCommandQueue());
            if (!queue) {
                throw std::runtime_error("Metal command queue creation failed");
            }

            return queue;
        }

        static NS::SharedPtr<MTL::Library> load_library(
            MTL::Device* device,
            const std::filesystem::path& path
        ) {
            const auto release_pool = NS::TransferPtr(NS::AutoreleasePool::alloc()->init());

            const auto* file_url = NS::URL::fileURLWithPath(
                NS::String::string(path.c_str(), NS::UTF8StringEncoding));

            NS::Error* load_error = nullptr;
            NS::SharedPtr<MTL::Library> current_library = NS::TransferPtr(
                device->newLibrary(file_url, &load_error));
            if (!current_library) {
                throw std::runtime_error(
                    describe_metal_error(load_error, "Shader library loading failed: " + path.string()));
            }

            return current_library;
        }

        [[nodiscard]] NS::SharedPtr<MTL::Buffer> allocate_buffer(
            const std::size_t bytes,
            const MTL::ResourceOptions options
        ) const {
            auto created_buffer = NS::TransferPtr(device_ptr->newBuffer(bytes, options));
            if (!created_buffer) {
                throw std::runtime_error("Metal buffer allocation failed when device was recognised");
            }

            return created_buffer;
        }
    };
} // namespace multithreading::graphics
