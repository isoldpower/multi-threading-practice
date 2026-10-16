#pragma once

#include <application/structures_scenarios/UnboundedQueueBenchmark.h>
#include <benchmark/mcmp/ProducerConsumerBenchmark.h>

namespace multithreading::structures_scenarios::mcmp {

    class UnboundedQueueMCMPBenchmark final
        : public multithreading::benchmark::mcmp::ProducerConsumerBenchmark,
          UnboundedQueueBenchmark {
    public:
        explicit UnboundedQueueMCMPBenchmark(
            const std::shared_ptr<multithreading::structures::unbounded_queue::UnboundedQueue<int>>&
                queue
        );

        void producer_routine(size_t threadSize) override;
        void consumer_routine(size_t threadSize) override;
    };
}  // namespace multithreading::structures_scenarios::mcmp