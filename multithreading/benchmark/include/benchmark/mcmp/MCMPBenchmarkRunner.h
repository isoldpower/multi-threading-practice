#pragma once

#include <benchmark/BenchmarkMatrix.h>
#include <benchmark/BenchmarkRunner.h>
#include <benchmark/mcmp/ProducerConsumerBenchmark.h>

#include <memory>

namespace multithreading::benchmark::mcmp {

    class MCMPBenchmarkRunner : public BenchmarkRunner {
    private:
        std::shared_ptr<ProducerConsumerBenchmark> benchmark;

    public:
        explicit MCMPBenchmarkRunner(const std::shared_ptr<ProducerConsumerBenchmark>& benchmark);

        void run_benchmark_with(const BenchmarkMatrixItem& item) override;
    };
}  // namespace multithreading::benchmark::mcmp
