#include <application/structures_scenarios/UnboundedQueueBenchmark.h>

namespace multithreading::structures_scenarios {

    UnboundedQueueBenchmark::UnboundedQueueBenchmark(
        const std::shared_ptr<multithreading::structures::unbounded_queue::UnboundedQueue<int>>&
            queue
    )
        : benchmark_queue(queue) {
    }
}  // namespace multithreading::structures_scenarios