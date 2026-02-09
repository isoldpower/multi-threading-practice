#pragma once

#include <memory>

#include "./ProducerConsumerBenchmark.h"
#include "../BenchmarkMatrix.h"
#include "../BenchmarkRunner.h"


namespace multithreading::benchmark::mcmp {

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
} // namespace multithreading::benchmark::mcmp
