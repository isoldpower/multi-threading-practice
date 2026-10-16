#include <application/computing_scenarios/merge_sort/MergeSortBenchmark.h>

namespace multithreading::computing_scenarios::merge_sort {

    MergeSortBenchmark::MergeSortBenchmark(
        const std::shared_ptr<multithreading::parallel_computing::merge_sort::MergeSort>& sort
    )
        : benchmark_sort(sort) {
    }

}  // namespace multithreading::computing_scenarios::merge_sort
