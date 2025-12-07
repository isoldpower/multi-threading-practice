#pragma once

#include "./BenchmarkMatrix.h"
#include "./BenchmarkRunner.h"


namespace multithreading::utilities::benchmark {

    struct alignas(64) BenchmarkResult {
        size_t threads_count;
        size_t thread_size;
        std::chrono::high_resolution_clock::duration execution_time;
    };

    class BenchmarkMeasurer {
    private:
        BenchmarkMatrixDefinition benchmark_matrix;
    public:
        explicit BenchmarkMeasurer(BenchmarkMatrixDefinition matrix)
            : benchmark_matrix(std::move(matrix))
        {}

        std::vector<BenchmarkResult> measure_benchmark(
            const std::shared_ptr<BenchmarkRunner>& benchmark_runner
        ) const;
    };
} // namespace multithreading::utilities::benchmark