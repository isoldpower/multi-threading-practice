#pragma once

#include <application/structures_scenarios/BoundedQueueBenchmark.h>
#include <benchmark/mcmp/ProducerConsumerBenchmark.h>

namespace multithreading::structures_scenarios::mcmp {

    class BoundedQueueMCMPBenchmark final
        : public multithreading::benchmark::mcmp::ProducerConsumerBenchmark,
          BoundedQueueBenchmark {
    public:
        explicit BoundedQueueMCMPBenchmark(
            const std::shared_ptr<multithreading::structures::bounded_queue::BoundedQueue<int>>&
                queue
        );

        void producer_routine(size_t threadSize) override;
        void consumer_routine(size_t threadSize) override;
    };
}  // namespace multithreading::structures_scenarios::mcmp