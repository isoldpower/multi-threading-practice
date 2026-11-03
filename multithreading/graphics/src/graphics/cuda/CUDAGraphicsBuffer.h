#pragma once

#include "../GraphicsBuffer.h"


namespace multithreading::graphics {

    class CUDAGraphicsBuffer : public GraphicsBuffer {
    public:
        CUDAGraphicsBuffer(const BufferMode mode) {
        }
    };
} // namespace multithreading::graphics