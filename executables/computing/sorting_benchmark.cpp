#include <application/computing_scenarios/merge_sort/MergeSortBenchmark.h>
#include <application/computing_scenarios/merge_sort/SortConfig.h>
#include <application/computing_scenarios/merge_sort/SortInputs.h>
#include <application/runtime/Application.h>
#include <benchmark/AverageMemoryMeasurement.h>
#include <benchmark/BenchmarkMatrix.h>
#include <benchmark/BenchmarkMeasurer.h>
#include <benchmark/BenchmarkMonitor.h>
#include <benchmark/BenchmarkRunner.h>
#include <benchmark/MultithreadingTask.h>
#include <benchmark/PeakMemoryMeasurement.h>
#include <benchmark/SpeedMeasurement.h>
#include <benchmark/views/ConsoleOutput.h>
#include <parallel_computing/merge_sort/ParallelMergeSort.h>
#include <parallel_computing/merge_sort/SyncMergeSort.h>

#include <cstddef>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// NOLINTBEGIN(google-build-using-namespace)
using namespace multithreading::benchmark;
using namespace multithreading::computing_scenarios::merge_sort;
// NOLINTEND(google-build-using-namespace)

namespace multithreading::application {

    using SortMonitor = BenchmarkMonitor<DurationType, double, double>;

    static std::shared_ptr<SortMonitor> make_monitor() {
        return std::make_shared<SortMonitor>(RefMeasurementsList<DurationType, double, double>(
            std::make_unique<SpeedMeasurement>(BenchmarkMeasurementTemplate{
                .verbose = std::string{"Execution Time", std::allocator<char>{}},
            }),
            std::make_unique<PeakMemoryMeasurement>(BenchmarkMeasurementTemplate{
                .verbose = std::string{"Peak Memory Overhead", std::allocator<char>{}},
            }),
            std::make_unique<AverageMemoryMeasurement>(BenchmarkMeasurementTemplate{
                .verbose = std::string{"Average Memory Usage", std::allocator<char>{}},
            })
        ));
    }

    static void run_task(
        const BenchmarkTask<BenchmarkRunner>& task,
        const BenchmarkMatrixDefinition& matrix,
        const std::shared_ptr<SortMonitor>& monitor
    ) {
        const BenchmarkMeasurer measurer(matrix, monitor);

        std::cout << task.title() << "\n";
        const auto results = measurer.measure_benchmark(task.structure());
        views::ConsoleOutput::displayBenchmarkResults(results);
    }

    static void benchmarkApplication() {
        const std::vector<std::size_t> sizes{
            {SMALL_INPUT_SIZE, MEDIUM_INPUT_SIZE, LARGE_INPUT_SIZE},
            std::allocator<std::size_t>{}
        };
        const auto inputs = std::make_shared<const SortInputs>(sizes, INPUT_SEED);
        const auto monitor = make_monitor();

        const BenchmarkTask<BenchmarkRunner> sync_sort(
            std::make_shared<MergeSortBenchmark>(
                [](std::size_t) -> std::unique_ptr<IntMergeSort> {
                    return std::make_unique<parallel_computing::merge_sort::SyncMergeSort<int>>();
                },
                inputs
            ),
            std::string{"Synchronous Merge Sort Benchmark", std::allocator<char>{}}
        );
        const BenchmarkTask<BenchmarkRunner> parallel_sort(
            std::make_shared<MergeSortBenchmark>(
                [](const std::size_t threads_count) -> std::unique_ptr<IntMergeSort> {
                    return std::make_unique<parallel_computing::merge_sort::ParallelMergeSort<int>>(
                        threads_count
                    );
                },
                inputs
            ),
            std::string{"Parallel Merge Sort Benchmark", std::allocator<char>{}}
        );

        run_task(
            sync_sort,
            BenchmarkMatrixDefinition{
                .per_thread_sizes = sizes,
                .threads_count = std::vector{{std::size_t{1}}, std::allocator<std::size_t>{}},
            },
            monitor
        );
        run_task(
            parallel_sort,
            BenchmarkMatrixDefinition{
                .per_thread_sizes = sizes,
                .threads_count = parallel_thread_counts(),
            },
            monitor
        );
    }
}  // namespace multithreading::application

auto main() -> int {
    try {
        multithreading::runtime::Application benchmarkApplication(
            multithreading::runtime::ApplicationInfo<int>{
                .appName = std::string{"Merge Sort Benchmark", std::allocator<char>{}},
                .appVersion = std::string{"1.0.0", std::allocator<char>{}},
                .beforeTask = std::nullopt,
                .afterTask = std::nullopt,
            }
        );

        const std::optional<int> executionResult = benchmarkApplication.SafeStart([] {
            multithreading::application::benchmarkApplication();
            return 0;
        });

        return executionResult.has_value() ? executionResult.value() : -1;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';

        return -1;
    } catch (...) {
        std::cerr << "Unknown exception\n";

        return -1;
    }
}
