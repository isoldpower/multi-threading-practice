#pragma once

#include "BenchmarkMatrix.h"

namespace multithreading::benchmark {

    class BenchmarkRunner {
    public:
        virtual ~BenchmarkRunner() = default;
        BenchmarkRunner() = default;

        BenchmarkRunner(const BenchmarkRunner&) = delete;
        BenchmarkRunner& operator=(const BenchmarkRunner&) = delete;
        BenchmarkRunner(BenchmarkRunner&&) = delete;
        BenchmarkRunner& operator=(BenchmarkRunner&&) = delete;

        virtual void run_benchmark_with(const BenchmarkMatrixItem& item) = 0;
    };
}  // namespace multithreading::benchmark
