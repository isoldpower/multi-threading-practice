#pragma once

#include <multithreading/structures/include/unbounded_queue/UnboundedQueue.h>

#include <memory>

namespace executables::benchmarks {

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

}  // namespace executables::benchmarks