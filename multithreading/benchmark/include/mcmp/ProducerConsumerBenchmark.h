#pragma once

#include <cstdio>


namespace multithreading::benchmark::mcmp {

    class ProducerConsumerBenchmark {
    public:
        virtual ~ProducerConsumerBenchmark() = default;

        virtual void producer_routine(std::size_t threadSize) = 0;
        virtual void consumer_routine(std::size_t threadSize) = 0;
    };
} // namespace multithreading::benchmark::mcmp