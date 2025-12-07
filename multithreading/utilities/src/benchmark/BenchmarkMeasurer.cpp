#include "../../include/benchmark/BenchmarkMeasurer.h"


namespace multithreading::utilities::benchmark {

    std::vector<BenchmarkResult> BenchmarkMeasurer::measure_benchmark(
        const std::shared_ptr<BenchmarkRunner> &benchmark_runner
    ) const {
        std::vector<BenchmarkResult> benchmark_results;

        for (const size_t threads_count : benchmark_matrix.threads_count) {
            for (const size_t thread_size : benchmark_matrix.per_thread_sizes) {
                const auto start_time = std::chrono::high_resolution_clock::now();
                benchmark_runner->run_benchmark_with({
                    .threads_count = threads_count,
                    .thread_size = thread_size
                });
                const auto end_time = std::chrono::high_resolution_clock::now();

                benchmark_results.emplace_back(BenchmarkResult{
                    .threads_count = threads_count,
                    .thread_size = thread_size,
                    .execution_time = std::chrono::duration_cast<std::chrono::high_resolution_clock::duration>(
                        end_time - start_time
                    )
                });
            }
        }

        return benchmark_results;
    }
} // namespace multithreading::utilities::benchmark