#pragma once

#include <memory>

#include "../BenchmarkMatrix.h"
#include "../BenchmarkRunner.h"
#include "./ProducerConsumerBenchmark.h"

namespace multithreading::benchmark::mcmp {

    class MCMPBenchmarkRunner : public BenchmarkRunner {
    private:
        std::shared_ptr<ProducerConsumerBenchmark> benchmark;

    public:
        explicit MCMPBenchmarkRunner(const std::shared_ptr<ProducerConsumerBenchmark>& benchmark);

        void run_benchmark_with(const BenchmarkMatrixItem& item) override;
    };
}  // namespace multithreading::benchmark::mcmp
