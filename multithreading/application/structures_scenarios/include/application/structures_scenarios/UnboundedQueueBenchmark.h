#pragma once

#include <structures/unbounded_queue/UnboundedQueue.h>

#include <memory>

namespace multithreading::structures_scenarios {

    class UnboundedQueueBenchmark {
    private:
        std::shared_ptr<multithreading::structures::unbounded_queue::UnboundedQueue<int>>
            benchmark_queue;

    protected:
        explicit UnboundedQueueBenchmark(
            const std::shared_ptr<multithreading::structures::unbounded_queue::UnboundedQueue<int>>&
                queue
        );

        [[nodiscard]] const std::shared_ptr<
            multithreading::structures::unbounded_queue::UnboundedQueue<int>>&
        queue() const noexcept {
            return benchmark_queue;
        }
    };

}  // namespace multithreading::structures_scenarios