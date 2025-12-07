#pragma once

#include <multithreading/structures/include/unbounded_queue/UnboundedQueue.h>

#include <memory>


namespace executables::benchmarks {

    class UnboundedQueueBenchmark {
    protected:
        std::shared_ptr<
            multithreading::structures::unbounded_queue::UnboundedQueue<int>
        > benchmark_queue;
    public:
        explicit UnboundedQueueBenchmark(
            const std::shared_ptr<
                multithreading::structures::unbounded_queue::UnboundedQueue<int>> &queue
        );
    };


} // namespace executables::benchmarks