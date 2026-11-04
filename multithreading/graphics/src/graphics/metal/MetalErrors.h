#pragma once

#include <Foundation/Foundation.hpp>

#include <string>


namespace multithreading::graphics {

    inline std::string describe_metal_error(const NS::Error* error, std::string fallback) {
        if (error != nullptr) {
            return error->localizedDescription()->utf8String();
        }

        return fallback;
    }
} // namespace multithreading::graphics
