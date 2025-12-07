#include "../include/BoundedQueueBenchmark.h"

namespace executables::benchmarks {

    BoundedQueueBenchmark::BoundedQueueBenchmark(
        const std::shared_ptr<
            multithreading::structures::bounded_queue::BoundedQueue<int>> &queue
    )
        : benchmark_queue(queue)
    {}
} // namespace executables::benchmarks