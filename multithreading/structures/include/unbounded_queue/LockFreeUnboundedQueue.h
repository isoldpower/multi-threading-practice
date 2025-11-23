#pragma once

#include "./UnboundedQueue.h"


namespace multithreading::structures::unbounded_queue {

    template <typename T>
    class LockFreeUnboundedQueue : public UnboundedQueue<T> {

    };
} // namespace multithreading::structures::unbounded_queue