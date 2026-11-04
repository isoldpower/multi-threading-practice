#pragma once

#include <parallel_computing/merge_sort/MergeSort.h>

#include <functional>
#include <memory>
#include <span>
#include <vector>

namespace multithreading::parallel_computing::merge_sort {

    template <typename T, typename Compare = std::less<T>>
    class SyncMergeSort final : public MergeSort<T, Compare> {
    public:
        using MergeSort<T, Compare>::perform_sort;

        void perform_sort(std::span<T> values, Compare compare) override {
            if (values.size() < 2) {
                return;
            }

            std::vector<T> buffer(std::allocator<T>{});
            buffer.resize(values.size());
            MergeSort<T, Compare>::sort_sequentially(values, buffer, 0, values.size(), compare);
        }
    };
}  // namespace multithreading::parallel_computing::merge_sort
