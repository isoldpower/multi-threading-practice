#include "../../include/mcmp/BoundedQueueMCMPBenchmark.h"

namespace executables::benchmarks::mcmp {

    constexpr size_t MAX_CONSUME_RETRIES = 100;

    BoundedQueueMCMPBenchmark::BoundedQueueMCMPBenchmark(
        const std::shared_ptr<
            multithreading::structures::bounded_queue::BoundedQueue<int>> &queue
    )
        : BoundedQueueBenchmark(queue)
    {}

    void BoundedQueueMCMPBenchmark::producer_routine(const size_t threadSize) {
        for (size_t j = 0; j < threadSize; j++) {
            while (!benchmark_queue->try_enqueue(static_cast<int>(j))) {
                // Keep trying until we have space
            }
        }
    }

    void BoundedQueueMCMPBenchmark::consumer_routine(const size_t threadSize) {
        for (size_t j = 0; j < threadSize; j++) {
            size_t retries = 0;
            while (!benchmark_queue->try_dequeue().has_value()) {
                if (++retries > MAX_CONSUME_RETRIES) {
                    std::this_thread::yield();
                    retries = 0;
                }
            }
        }
    }

} // namespace executables::benchmarks::mcmp