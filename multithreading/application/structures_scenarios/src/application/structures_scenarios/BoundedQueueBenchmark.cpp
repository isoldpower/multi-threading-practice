#include <application/structures_scenarios/BoundedQueueBenchmark.h>

namespace multithreading::structures_scenarios {

    BoundedQueueBenchmark::BoundedQueueBenchmark(
        const std::shared_ptr<multithreading::structures::bounded_queue::BoundedQueue<int>>& queue
    )
        : benchmark_queue(queue) {
    }
}  // namespace multithreading::structures_scenarios