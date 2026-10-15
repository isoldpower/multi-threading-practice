#pragma once

#include <multithreading/structures/include/bounded_queue/BoundedQueue.h>

#include <memory>

namespace executables::benchmarks {

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

}  // namespace executables::benchmarks