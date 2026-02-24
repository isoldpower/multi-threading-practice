#pragma once

#include <multithreading/benchmark/include/mcmp/ProducerConsumerBenchmark.h>

#include "../BoundedQueueBenchmark.h"

namespace executables::benchmarks::mcmp {

    class BoundedQueueMCMPBenchmark final
        : public multithreading::benchmark::mcmp::ProducerConsumerBenchmark
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