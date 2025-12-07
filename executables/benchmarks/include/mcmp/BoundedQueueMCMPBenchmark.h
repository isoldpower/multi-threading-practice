#pragma once

#include <multithreading/utilities/include/benchmark/ProducerConsumerBenchmark.h>

#include "../BoundedQueueBenchmark.h"

namespace executables::benchmarks::mcmp {

    class BoundedQueueMCMPBenchmark final
        : public multithreading::utilities::benchmark::ProducerConsumerBenchmark
        , BoundedQueueBenchmark
    {
    public:
        explicit BoundedQueueMCMPBenchmark(
            const std::shared_ptr<
                multithreading::structures::bounded_queue::BoundedQueue<int>> &queue
        );

        void producer_routine(size_t threadSize) override;
        void consumer_routine(size_t threadSize) override;
    };
} // namespace executables::benchmarks::mcmp