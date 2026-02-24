#pragma once

#include <multithreading/benchmark/include/mcmp/ProducerConsumerBenchmark.h>

#include "../UnboundedQueueBenchmark.h"

namespace executables::benchmarks::mcmp {

    class UnboundedQueueMCMPBenchmark final
        : public multithreading::benchmark::mcmp::ProducerConsumerBenchmark
        , UnboundedQueueBenchmark
    {
    public:
        explicit UnboundedQueueMCMPBenchmark(
            const std::shared_ptr<
                multithreading::structures::unbounded_queue::UnboundedQueue<int>> &queue
        );

        void producer_routine(size_t threadSize) override;
        void consumer_routine(size_t threadSize) override;
    };
} // namespace executables::benchmarks::mcmp