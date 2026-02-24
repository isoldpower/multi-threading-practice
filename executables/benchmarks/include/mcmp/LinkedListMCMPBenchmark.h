#pragma once

#include <multithreading/structures/include/linked_list/FGLockLinkedList.h>
#include <multithreading/benchmark/include/mcmp/ProducerConsumerBenchmark.h>
#include <memory>

#include "../LinkedListBenchmark.h"


namespace executables::benchmarks::mcmp {

    class LinkedListMCMPBenchmark final
        : public multithreading::benchmark::mcmp::ProducerConsumerBenchmark
        , LinkedListBenchmark
    {
    public:
        explicit LinkedListMCMPBenchmark(
            const std::shared_ptr<
                multithreading::structures::linked_list::LinkedList<int>
            > &list
        );

        void producer_routine(size_t threadSize) override;
        void consumer_routine(size_t threadSize) override;
    };
} // namespace executables::benchmarks::mcmp