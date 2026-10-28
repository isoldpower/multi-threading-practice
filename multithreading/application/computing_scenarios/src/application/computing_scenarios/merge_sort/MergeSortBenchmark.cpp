#include <application/computing_scenarios/merge_sort/MergeSortBenchmark.h>

#include <utility>
#include <vector>

namespace multithreading::computing_scenarios::merge_sort {

    MergeSortBenchmark::MergeSortBenchmark(
        SorterFactory sorter_factory,
        std::shared_ptr<const SortInputs> inputs
    )
        : sorter_factory(std::move(sorter_factory))
        , inputs(std::move(inputs)) {
    }

    void MergeSortBenchmark::run_benchmark_with(const benchmark::BenchmarkMatrixItem& item) {
        std::vector<int> values = inputs->of_size(item.thread_size);
        const std::unique_ptr<IntMergeSort> sorter = sorter_factory(item.threads_count);

        sorter->perform_sort(values);
    }
}  // namespace multithreading::computing_scenarios::merge_sort
