#pragma once

#include <structures/bounded_queue/BoundedQueue.h>

#include <memory>

namespace multithreading::structures_scenarios {

    class BoundedQueueBenchmark {
    private:
        std::shared_ptr<multithreading::structures::bounded_queue::BoundedQueue<int>>
            benchmark_queue;

    protected:
        explicit BoundedQueueBenchmark(
            const std::shared_ptr<multithreading::structures::bounded_queue::BoundedQueue<int>>&
                queue
        );

        [[nodiscard]] const std::shared_ptr<
            multithreading::structures::bounded_queue::BoundedQueue<int>>&
        queue() const noexcept {
            return benchmark_queue;
        }
    };

}  // namespace multithreading::structures_scenarios