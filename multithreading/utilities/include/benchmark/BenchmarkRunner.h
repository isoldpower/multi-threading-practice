#pragma once

#include "./BenchmarkMatrix.h"


namespace multithreading::utilities::benchmark {

    class BenchmarkRunner {
    public:
        virtual ~BenchmarkRunner() = default;
        BenchmarkRunner() = default;

        virtual void run_benchmark_with(
            const BenchmarkMatrixItem& item
        ) = 0;
    };
} // namespace multithreading::utilities::benchmark
