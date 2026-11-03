#pragma once

#include <filesystem>
#include <memory>

#include "./GraphicsDevice.h"


namespace multithreading::graphics {

    struct GraphicsOptions {  // NOLINT(altera-struct-pack-align)
        std::filesystem::path shader_library_path;
    };

    class GraphicsFactory {
    public:
        static std::unique_ptr<GraphicsDevice> build_graphics(const GraphicsOptions& options);
    };
} // namespace multithreading::graphics
