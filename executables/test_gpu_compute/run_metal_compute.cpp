#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

#include <exception>
#include <filesystem>
#include <iostream>
#include <mach-o/dyld.h>
#include <span>
#include <string>
#include <vector>


static std::filesystem::path executable_directory() {
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::string buffer(size, '\0');
    _NSGetExecutablePath(buffer.data(), &size);

    const std::filesystem::path raw_path(buffer.c_str(), std::filesystem::path::auto_format);
    return std::filesystem::canonical(raw_path).parent_path();
}

static std::filesystem::path build_shader_path(const std::filesystem::path &current_directory) {
    const auto path_suffix = std::filesystem::path(
        SHADER_LIBRARY_NAME,
        std::filesystem::path::auto_format);

    return current_directory / path_suffix;
}

static void print_error(const char* stage, const NS::Error* error) {
    std::cerr << stage << " failed";
    if (error != nullptr) {
        std::cerr << ": " << error->localizedDescription()->utf8String();
    }
    std::cerr << '\n';
}

static NS::SharedPtr<MTL::Library> build_library(
    const NS::SharedPtr<MTL::Device> &gpu_device,
    NS::Error* error
) {
    const std::string shaders_lib_path = build_shader_path(executable_directory()).string();
    const auto* file_url = NS::URL::fileURLWithPath(
        NS::String::string(shaders_lib_path.c_str(), NS::UTF8StringEncoding));

    return NS::TransferPtr(gpu_device->newLibrary(file_url, &error));
}

static NS::SharedPtr<MTL::ComputePipelineState> create_pipeline(
    const NS::SharedPtr<MTL::Device> &gpu_device,
    const NS::SharedPtr<MTL::Library> &test_library,
    NS::Error* error
) {
    const auto add_function = NS::TransferPtr(
        test_library->newFunction(NS::String::string("add_one", NS::UTF8StringEncoding)));

    return NS::TransferPtr(gpu_device->newComputePipelineState(add_function.get(), &error));
}

static void* run_pipeline_on_device(
    const NS::SharedPtr<MTL::ComputePipelineState>& pipeline,
    const NS::SharedPtr<MTL::Device>& gpu_device,
    const std::vector<float>& input
) {
    const NS::UInteger bytes = input.size() * sizeof(float);
    const auto in_array = NS::TransferPtr(
        gpu_device->newBuffer(input.data(), bytes, MTL::ResourceStorageModeShared));
    const auto out_array = NS::TransferPtr(gpu_device->newBuffer(bytes, MTL::ResourceStorageModeShared));

    const auto queue = NS::TransferPtr(gpu_device->newCommandQueue());
    MTL::CommandBuffer* commands = queue->commandBuffer();
    MTL::ComputeCommandEncoder* encoder = commands->computeCommandEncoder();
    encoder->setComputePipelineState(pipeline.get());
    encoder->setBuffer(in_array.get(), 0, 0);
    encoder->setBuffer(out_array.get(), 0, 1);
    encoder->dispatchThreads(
        MTL::Size(input.size(), 1, 1),
        MTL::Size(pipeline->maxTotalThreadsPerThreadgroup(), 1, 1));
    encoder->endEncoding();

    commands->commit();
    commands->waitUntilCompleted();

    return out_array->contents();
}

static int run_gpu_application() {
    NS::Error* load_error = nullptr;
    const NS::SharedPtr<NS::AutoreleasePool> release_pool =
        NS::TransferPtr(NS::AutoreleasePool::alloc()->init());

    const NS::SharedPtr<MTL::Device> gpu_device = NS::TransferPtr(MTL::CreateSystemDefaultDevice());
    if (!gpu_device) {
        print_error("Device creation", nullptr);
        return 1;
    }

    const NS::SharedPtr<MTL::Library> test_library = build_library(gpu_device, load_error);
    if (!test_library) {
        print_error("Library loading", load_error);
        return 1;
    }

    const auto pipeline = create_pipeline(gpu_device, test_library, load_error);
    if (!pipeline) {
        print_error("Pipeline creation", load_error);
        return 1;
    }

    const std::vector input = {1.5F, 2.4F, 3.2F, 4.5F, 5.9F};
    const void* contents = run_pipeline_on_device(pipeline, gpu_device, input);

    const std::span result(static_cast<const float*>(contents), input.size());
    for (const float value : result) {
        std::cout << value << ' ';
    }

    std::cout << '\n';
    return 0;
}

auto main() -> int {
      try {
          return run_gpu_application();
      } catch (const std::exception& exception) {
          std::cerr << "Unhandled exception: " << exception.what() << '\n';
      } catch (...) {
          std::cerr << "Unhandled unknown exception\n";
      }
      return 1;
  }