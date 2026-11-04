#include <application/structures_scenarios/Backoff.h>
#include <application/structures_scenarios/mcmp/BoundedQueueMCMPBenchmark.h>

namespace multithreading::structures_scenarios::mcmp {

    BoundedQueueMCMPBenchmark::BoundedQueueMCMPBenchmark(
        const std::shared_ptr<structures::bounded_queue::BoundedQueue<int>>& queue)
        : BoundedQueueBenchmark(queue) {
    }

    void BoundedQueueMCMPBenchmark::producer_routine(const size_t threadSize) {
        for (size_t j = 0; j < threadSize; j++) {
            retry_with_backoff([&] {
                return queue()->try_enqueue(static_cast<int>(j));
            });
        }
    }

    void BoundedQueueMCMPBenchmark::consumer_routine(const size_t threadSize) {
        for (size_t j = 0; j < threadSize; j++) {
            retry_with_backoff([&] {
                return queue()->try_dequeue().has_value();
            });
        }
    }

}  // namespace multithreading::structures_scenarios::mcmp
