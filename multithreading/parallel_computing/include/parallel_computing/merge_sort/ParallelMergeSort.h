#pragma once

#include <parallel_computing/merge_sort/MergeSort.h>

#include <bit>
#include <cstddef>
#include <exception>
#include <functional>
#include <span>
#include <stdexcept>
#include <thread>
#include <vector>

namespace multithreading::parallel_computing::merge_sort {

    template <typename T, typename Compare = std::less<T>>
    class ParallelMergeSort final : public MergeSort<T, Compare> {
    public:
        static constexpr std::size_t DEFAULT_MIN_PARALLEL_SIZE = 4096;

        explicit ParallelMergeSort(const std::size_t thread_count)
            : ParallelMergeSort(thread_count, DEFAULT_MIN_PARALLEL_SIZE) {
        }

        ParallelMergeSort(const std::size_t thread_count, const std::size_t min_parallel_size)
            : max_fork_depth(fork_depth_for(thread_count))
            , min_parallel_size(min_parallel_size) {
        }

        using MergeSort<T, Compare>::perform_sort;

        void perform_sort(std::span<T> values, Compare compare) override {
            if (values.size() < 2) {
                return;
            }

            std::vector<T> buffer(values.size());
            fork_and_sort(values, buffer, 0, values.size(), 0, compare);
        }

    private:
        std::size_t max_fork_depth;
        std::size_t min_parallel_size;

        static std::size_t fork_depth_for(const std::size_t thread_count) {
            if (thread_count == 0) {
                throw std::invalid_argument("ParallelMergeSort needs at least one thread");
            }

            return static_cast<std::size_t>(std::bit_width(thread_count)) - 1;
        }

        void fork_and_sort(
            std::span<T> values,
            std::span<T> buffer,
            const std::size_t left,
            const std::size_t right,
            const std::size_t depth,
            Compare& compare
        ) const {
            const std::size_t size = right - left;
            if (depth >= max_fork_depth || size < min_parallel_size) {
                MergeSort<T, Compare>::sort_sequentially(values, buffer, left, right, compare);
                return;
            }

            // Sort left half of array async while we sort the right half sync
            const std::size_t middle = left + (size / 2);
            std::exception_ptr forked_error;
            std::thread forked(
                [this, values, buffer, left, middle, depth, &forked_error](Compare forked_compare) {
                    try {
                        fork_and_sort(values, buffer, left, middle, depth + 1, forked_compare);
                    } catch (...) {
                        forked_error = std::current_exception();
                    }
                },
                compare
            );

            // Sort right half of array synchronously
            try {
                fork_and_sort(values, buffer, middle, right, depth + 1, compare);
            } catch (...) {
                forked.join();
                throw;
            }

            // Wait for the left side to be sorted
            forked.join();
            if (forked_error) {
                std::rethrow_exception(forked_error);
            }

            MergeSort<T, Compare>::merge_chunks(values, buffer, left, middle, right, compare);
        }
    };
}  // namespace multithreading::parallel_computing::merge_sort
