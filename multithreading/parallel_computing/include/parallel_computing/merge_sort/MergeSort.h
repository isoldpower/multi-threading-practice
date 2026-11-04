#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <span>

namespace multithreading::parallel_computing::merge_sort {

    template <typename T, typename Compare = std::less<T>>
    class MergeSort {
    public:
        using value_type = T;
        using compare_type = Compare;

        MergeSort() = default;
        virtual ~MergeSort() = default;

        MergeSort(const MergeSort&) = delete;
        MergeSort& operator=(const MergeSort&) = delete;
        MergeSort(MergeSort&&) = delete;
        MergeSort& operator=(MergeSort&&) = delete;

        virtual void perform_sort(std::span<T> values, Compare compare) = 0;

        void perform_sort(std::span<T> values) {
            perform_sort(values, Compare{});
        }

    protected:
        static void sort_sequentially(
            std::span<T> values,
            std::span<T> buffer,
            const std::size_t left,
            const std::size_t right,
            Compare& compare
        ) {
            if (right - left < 2) {
                return;
            }

            const std::size_t middle = left + ((right - left) / 2);
            sort_sequentially(values, buffer, left, middle, compare);
            sort_sequentially(values, buffer, middle, right, compare);

            merge_chunks(values, buffer, left, middle, right, compare);
        }

        static void merge_chunks(
            std::span<T> values,
            std::span<T> buffer,
            const std::size_t left,
            const std::size_t middle,
            const std::size_t right,
            Compare& compare
        ) {
            const auto values_begin = values.begin();
            const auto buffer_begin = buffer.begin();

            std::move(values_begin + left, values_begin + right, buffer_begin + left);

            auto left_run = buffer_begin + left;
            const auto left_end = buffer_begin + middle;
            auto right_run = left_end;
            const auto right_end = buffer_begin + right;
            auto merged = values_begin + left;

            while (left_run != left_end && right_run != right_end) {
                if (compare(*right_run, *left_run)) {
                    *merged = std::move(*right_run);
                    ++right_run;
                } else {
                    *merged = std::move(*left_run);
                    ++left_run;
                }
                ++merged;
            }

            merged = std::move(left_run, left_end, merged);
            std::move(right_run, right_end, merged);
        }
    };
}  // namespace multithreading::parallel_computing::merge_sort
