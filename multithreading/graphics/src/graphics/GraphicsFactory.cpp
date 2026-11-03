#include <graphics/GraphicsFactory.h>
#include <utility>

#ifdef MT_HAS_METAL
#include "./metal/MetalGraphicsDevice.h"
#endif

namespace multithreading::graphics {

    std::unique_ptr<GraphicsDevice> GraphicsFactory::build_graphics(const GraphicsOptions& options) {
#ifdef MT_HAS_METAL
        if (auto metal_device = NS::TransferPtr(MTL::CreateSystemDefaultDevice())) {
            return std::make_unique<MetalGraphicsDevice>(
                std::move(metal_device), options.shader_library_path);
        }
#else
        static_cast<void>(options);
#endif

        return nullptr;
    }
} // namespace multithreading::graphics
