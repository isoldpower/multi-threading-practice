#pragma once

#include <benchmark/BenchmarkMatrix.h>
#include <benchmark/BenchmarkMeasurement.h>
#include <benchmark/BenchmarkMonitor.h>
#include <benchmark/BenchmarkRunner.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <tuple>
#include <utility>
#include <vector>

namespace multithreading::benchmark {

    template <typename... TMeasures>
    struct BenchmarkResult {  // NOLINT(altera-struct-pack-align)
        size_t threads_count;
        size_t thread_size;
        std::tuple<BenchmarkMeasurementResult<TMeasures>...> measurements;
    };

    template <typename... TMeasures>
    class BenchmarkMeasurer {
    private:
        BenchmarkMatrixDefinition benchmark_matrix;
        std::shared_ptr<BenchmarkMonitor<TMeasures...>> benchmark_monitor;

        /** @brief Runs the largest cell once, unmeasured, so the first measured run does not
         * alone pay for cold caches, first-touch page faults and the allocator growing its heap. */
        void warm_up(const std::shared_ptr<BenchmarkRunner>& benchmark_runner) const {
            if (benchmark_matrix.threads_count.empty() ||
                benchmark_matrix.per_thread_sizes.empty()) {
                return;
            }

            benchmark_runner->run_benchmark_with(
                {.threads_count = std::ranges::max(
                     benchmark_matrix.threads_count,
                     std::ranges::less{},
                     std::identity{}
                 ),
                 .thread_size = std::ranges::max(
                     benchmark_matrix.per_thread_sizes,
                     std::ranges::less{},
                     std::identity{}
                 )}
            );
        }

    public:
        explicit BenchmarkMeasurer(
            BenchmarkMatrixDefinition matrix,
            std::shared_ptr<BenchmarkMonitor<TMeasures...>> monitor
        )
            : benchmark_matrix(std::move(matrix))
            , benchmark_monitor(std::move(monitor)) {
        }

        [[nodiscard]] std::vector<BenchmarkResult<TMeasures...>> measure_benchmark(
            const std::shared_ptr<BenchmarkRunner>& benchmark_runner
        ) const {
            std::vector<BenchmarkResult<TMeasures...>> benchmark_results;
            warm_up(benchmark_runner);

            for (const size_t threads_count : benchmark_matrix.threads_count) {
                for (const size_t thread_size : benchmark_matrix.per_thread_sizes) {
                    this->benchmark_monitor->start_monitoring();
                    benchmark_runner->run_benchmark_with(
                        {.threads_count = threads_count, .thread_size = thread_size}
                    );
                    this->benchmark_monitor->stop_monitoring();

                    if (auto measurements = this->benchmark_monitor->get_results();
                        measurements.has_value()) {
                        benchmark_results.emplace_back(
                            BenchmarkResult<TMeasures...>{
                                .threads_count = threads_count,
                                .thread_size = thread_size,
                                .measurements = measurements.value()
                            }
                        );
                    }
                }
            }

            return benchmark_results;
        }
    };
}  // namespace multithreading::benchmark
