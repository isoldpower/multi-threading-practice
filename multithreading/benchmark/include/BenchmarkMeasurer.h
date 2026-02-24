#pragma once

#include "./BenchmarkMatrix.h"
#include "./BenchmarkMeasurement.h"
#include "./BenchmarkMonitor.h"
#include "./BenchmarkRunner.h"

#include <memory>

namespace multithreading::benchmark {

    template <typename ...TMeasures>
    struct alignas(64) BenchmarkResult {
        size_t threads_count;
        size_t thread_size;
        std::tuple<BenchmarkMeasurementResult<TMeasures>...> measurements;
    };

    template <typename ...TMeasures>
    class BenchmarkMeasurer {
    private:
        BenchmarkMatrixDefinition benchmark_matrix;
        std::shared_ptr<BenchmarkMonitor<TMeasures...>> benchmark_monitor;
    public:
        explicit BenchmarkMeasurer(
            BenchmarkMatrixDefinition matrix,
            std::shared_ptr<BenchmarkMonitor<TMeasures...>> monitor
        )
            : benchmark_matrix(std::move(matrix))
            , benchmark_monitor(monitor)
        {}

        [[nodiscard]] std::vector<BenchmarkResult<TMeasures...>> measure_benchmark(
            const std::shared_ptr<BenchmarkRunner>& benchmark_runner
        ) const {
            std::vector<BenchmarkResult<TMeasures...>> benchmark_results;

            for (const size_t threads_count : benchmark_matrix.threads_count) {
                for (const size_t thread_size : benchmark_matrix.per_thread_sizes) {
                    this->benchmark_monitor->start_monitoring();
                    benchmark_runner->run_benchmark_with({
                        .threads_count = threads_count,
                        .thread_size = thread_size
                    });
                    this->benchmark_monitor->stop_monitoring();

                    if (auto measurements = this->benchmark_monitor->get_results(); measurements.has_value()) {
                        benchmark_results.emplace_back(BenchmarkResult<TMeasures...>{
                            .threads_count = threads_count,
                            .thread_size = thread_size,
                            .measurements = measurements.value()
                        });
                    }
                }
            }

            return benchmark_results;
        }
    };
} // namespace multithreading::benchmark
