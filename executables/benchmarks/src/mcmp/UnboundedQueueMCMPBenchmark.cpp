#include "../../include/mcmp/UnboundedQueueMCMPBenchmark.h"

namespace executables::benchmarks::mcmp {

    UnboundedQueueMCMPBenchmark::UnboundedQueueMCMPBenchmark(
        const std::shared_ptr<
            multithreading::structures::unbounded_queue::UnboundedQueue<int>> &queue
    )
        : UnboundedQueueBenchmark(queue)
    {}

    void UnboundedQueueMCMPBenchmark::producer_routine(const size_t threadSize) {
        for (size_t j = 0; j < threadSize; j++) {
            benchmark_queue->enqueue(static_cast<int>(j));
        }
    }

    void UnboundedQueueMCMPBenchmark::consumer_routine(const size_t threadSize) {
        for (size_t j = 0; j < threadSize; j++) {
            while (!benchmark_queue->try_dequeue().has_value()) {
                // Keep trying until we get a value
            }
        }
    }

} // namespace executables::benchmarks::mcmp