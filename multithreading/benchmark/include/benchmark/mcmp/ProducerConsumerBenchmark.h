#pragma once

#include <cstdio>

namespace multithreading::benchmark::mcmp {

    class ProducerConsumerBenchmark {
    public:
        virtual ~ProducerConsumerBenchmark() = default;
        ProducerConsumerBenchmark() = default;

        ProducerConsumerBenchmark(const ProducerConsumerBenchmark&) = delete;
        ProducerConsumerBenchmark& operator=(const ProducerConsumerBenchmark&) = delete;
        ProducerConsumerBenchmark(ProducerConsumerBenchmark&&) = delete;
        ProducerConsumerBenchmark& operator=(ProducerConsumerBenchmark&&) = delete;

        virtual void producer_routine(std::size_t threadSize) = 0;
        virtual void consumer_routine(std::size_t threadSize) = 0;
    };
}  // namespace multithreading::benchmark::mcmp