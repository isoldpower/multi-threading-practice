#pragma once

#include "../BenchmarkMatrix.h"
#include "../ProducerConsumerBenchmark.h"
#include "../BenchmarkRunner.h"
#include <memory>


namespace multithreading::utilities::benchmark::mcmp {

    class MCMPBenchmarkRunner : public BenchmarkRunner {
    private:
        std::shared_ptr<ProducerConsumerBenchmark> benchmark;
    public:
        explicit MCMPBenchmarkRunner(
            const std::shared_ptr<ProducerConsumerBenchmark>& benchmark
        );

        ~MCMPBenchmarkRunner() override = default;

        void run_benchmark_with(
            const BenchmarkMatrixItem& item
        ) override;
    };
} // namespace multithreading::utilities::benchmark::mcmp
