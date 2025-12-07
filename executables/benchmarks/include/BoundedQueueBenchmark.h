#pragma once

#include <multithreading/structures/include/bounded_queue/BoundedQueue.h>

#include <memory>


namespace executables::benchmarks {

    class BoundedQueueBenchmark {
    protected:
        std::shared_ptr<
            multithreading::structures::bounded_queue::BoundedQueue<int>
        > benchmark_queue;
    public:
        explicit BoundedQueueBenchmark(
            const std::shared_ptr<
                multithreading::structures::bounded_queue::BoundedQueue<int>> &queue
        );
    };


} // namespace executables::benchmarks