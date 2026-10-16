#pragma once

#include <parallel_computing/merge_sort/MergeSort.h>

#include <memory>

namespace multithreading::computing_scenarios::merge_sort {

    class MergeSortBenchmark {
    private:
        std::shared_ptr<parallel_computing::merge_sort::MergeSort> benchmark_sort;

    protected:
        explicit MergeSortBenchmark(
            const std::shared_ptr<parallel_computing::merge_sort::MergeSort>& sort
        );

        [[nodiscard]] const std::shared_ptr<
            parallel_computing::merge_sort::MergeSort>&
        sort() const noexcept {
            return benchmark_sort;
        }
    };

}  // namespace multithreading::computing_scenarios::merge_sort
