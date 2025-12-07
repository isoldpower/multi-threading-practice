#include "../include/UnboundedQueueBenchmark.h"

namespace executables::benchmarks {

    UnboundedQueueBenchmark::UnboundedQueueBenchmark(
        const std::shared_ptr<
            multithreading::structures::unbounded_queue::UnboundedQueue<int>> &queue
    )
        : benchmark_queue(queue)
    {}
} // namespace executables::benchmarks