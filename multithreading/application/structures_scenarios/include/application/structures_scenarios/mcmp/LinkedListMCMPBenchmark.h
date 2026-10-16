#pragma once

#include <application/structures_scenarios/LinkedListBenchmark.h>
#include <benchmark/mcmp/ProducerConsumerBenchmark.h>
#include <structures/linked_list/FGLockLinkedList.h>

#include <memory>

namespace multithreading::structures_scenarios::mcmp {

    class LinkedListMCMPBenchmark final
        : public multithreading::benchmark::mcmp::ProducerConsumerBenchmark,
          LinkedListBenchmark {
    public:
        explicit LinkedListMCMPBenchmark(
            const std::shared_ptr<multithreading::structures::linked_list::LinkedList<int> >& list
        );

        void producer_routine(size_t threadSize) override;
        void consumer_routine(size_t threadSize) override;
    };
}  // namespace multithreading::structures_scenarios::mcmp