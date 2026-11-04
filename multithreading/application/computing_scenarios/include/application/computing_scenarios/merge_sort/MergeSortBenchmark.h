#pragma once

#include <application/computing_scenarios/merge_sort/SortInputs.h>
#include <benchmark/BenchmarkMatrix.h>
#include <benchmark/BenchmarkRunner.h>
#include <parallel_computing/merge_sort/MergeSort.h>

#include <cstddef>
#include <functional>
#include <memory>

namespace multithreading::computing_scenarios::merge_sort {

    using IntMergeSort = parallel_computing::merge_sort::MergeSort<int>;

    using SorterFactory = std::function<std::unique_ptr<IntMergeSort>(std::size_t threads_count)>;

    class MergeSortBenchmark final : public benchmark::BenchmarkRunner {
    private:
        SorterFactory sorter_factory;
        std::shared_ptr<const SortInputs> inputs;

    public:
        MergeSortBenchmark(SorterFactory sorter_factory, std::shared_ptr<const SortInputs> inputs);

        void run_benchmark_with(const benchmark::BenchmarkMatrixItem& item) override;
    };
}  // namespace multithreading::computing_scenarios::merge_sort
