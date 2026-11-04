#include <application/computing_scenarios/merge_sort/MergeSortBenchmark.h>
#include <application/computing_scenarios/merge_sort/SortConfig.h>
#include <application/computing_scenarios/merge_sort/SortScenarios.h>
#include <benchmark/AverageMemoryMeasurement.h>
#include <benchmark/PeakMemoryMeasurement.h>
#include <benchmark/SpeedMeasurement.h>
#include <parallel_computing/merge_sort/GPUBitonicSort.h>
#include <parallel_computing/merge_sort/GPUMergeSort.h>
#include <parallel_computing/merge_sort/GPUSortContext.h>
#include <parallel_computing/merge_sort/ParallelMergeSort.h>
#include <parallel_computing/merge_sort/SyncMergeSort.h>

#include <filesystem>
#include <iostream>
#include <string>
#include <utility>


namespace multithreading::computing_scenarios::merge_sort {

    [[nodiscard]] static std::vector<std::size_t> single_thread() {
        return std::vector{{std::size_t{1}}, std::allocator<std::size_t>{}};
    }

    static void add_scenario(
        std::vector<SortScenario>& scenarios,
        std::string title,
        SorterFactory sorter_factory,
        const std::shared_ptr<const SortInputs>& inputs,
        benchmark::BenchmarkMatrixDefinition matrix
    ) {
        scenarios.push_back(SortScenario{
            .task = benchmark::BenchmarkTask<benchmark::BenchmarkRunner>(
                std::make_shared<MergeSortBenchmark>(std::move(sorter_factory), inputs),
                std::move(title)
            ),
            .matrix = std::move(matrix),
        });
    }

    std::shared_ptr<SortMonitor> make_sort_monitor() {
        return std::make_shared<SortMonitor>(
            benchmark::RefMeasurementsList<benchmark::DurationType, double, double>(
                std::make_unique<benchmark::SpeedMeasurement>(benchmark::BenchmarkMeasurementTemplate{
                    .verbose = std::string{"Execution Time", std::allocator<char>{}},
                }),
                std::make_unique<benchmark::PeakMemoryMeasurement>(
                    benchmark::BenchmarkMeasurementTemplate{
                        .verbose = std::string{"Peak Memory Overhead", std::allocator<char>{}},
                    }
                ),
                std::make_unique<benchmark::AverageMemoryMeasurement>(
                    benchmark::BenchmarkMeasurementTemplate{
                        .verbose = std::string{"Average Memory Usage", std::allocator<char>{}},
                    }
                )
            )
        );
    }

    static void add_gpu_scenarios(
        std::vector<SortScenario>& scenarios,
        const std::vector<std::size_t>& sizes,
        const std::shared_ptr<const SortInputs>& inputs
    ) {
#ifdef MT_GPU_SORT_LIBRARY
        const std::filesystem::path shader_library(
            MT_GPU_SORT_LIBRARY,
            std::filesystem::path::auto_format
        );
        const std::shared_ptr<const parallel_computing::merge_sort::GPUSortContext> context =
            parallel_computing::merge_sort::GPUSortContext::create(shader_library);

        if (context) {
            add_scenario(
                scenarios,
                std::string{"GPU Bitonic Sort Benchmark", std::allocator<char>{}},
                [context](std::size_t) -> std::unique_ptr<IntMergeSort> {
                    return std::make_unique<parallel_computing::merge_sort::GPUBitonicSort<int>>(
                        context
                    );
                },
                inputs,
                {.per_thread_sizes = sizes, .threads_count = single_thread()}
            );
            add_scenario(
                scenarios,
                std::string{"GPU Merge Sort Benchmark", std::allocator<char>{}},
                [context](std::size_t) -> std::unique_ptr<IntMergeSort> {
                    return std::make_unique<parallel_computing::merge_sort::GPUMergeSort<int>>(
                        context
                    );
                },
                inputs,
                {.per_thread_sizes = sizes, .threads_count = single_thread()}
            );
        } else {
            std::cout << "No supported GPU found, skipping GPU sort benchmarks\n";
        }
#else
        static_cast<void>(scenarios);
        static_cast<void>(sizes);
        static_cast<void>(inputs);
        std::cout << "Built without GPU support, skipping GPU sort benchmarks\n";
#endif
    }

    std::vector<SortScenario> make_sort_scenarios(
        const std::vector<std::size_t>& sizes,
        const std::shared_ptr<const SortInputs>& inputs
    ) {
        std::vector<SortScenario> scenarios;

        add_scenario(
            scenarios,
            std::string{"Synchronous Merge Sort Benchmark", std::allocator<char>{}},
            [](std::size_t) -> std::unique_ptr<IntMergeSort> {
                return std::make_unique<parallel_computing::merge_sort::SyncMergeSort<int>>();
            },
            inputs,
            {.per_thread_sizes = sizes, .threads_count = single_thread()}
        );
        add_scenario(
            scenarios,
            std::string{"Parallel Merge Sort Benchmark", std::allocator<char>{}},
            [](const std::size_t threads_count) -> std::unique_ptr<IntMergeSort> {
                return std::make_unique<parallel_computing::merge_sort::ParallelMergeSort<int>>(
                    threads_count
                );
            },
            inputs,
            {.per_thread_sizes = sizes, .threads_count = parallel_thread_counts()}
        );

        add_gpu_scenarios(scenarios, sizes, inputs);

        return scenarios;
    }
}  // namespace multithreading::computing_scenarios::merge_sort
