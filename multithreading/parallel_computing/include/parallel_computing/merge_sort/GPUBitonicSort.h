#pragma once

#include <parallel_computing/merge_sort/GPUSortContext.h>
#include <parallel_computing/merge_sort/MergeSort.h>

#include <graphics/GraphicsBuffer.h>
#include <graphics/GraphicsCommandList.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace multithreading::parallel_computing::merge_sort {

    template <typename T, typename Compare = std::less<T>>
        requires GPUSortable<T> && GPUSortOrder<Compare, T>
    class GPUBitonicSort final : public MergeSort<T, Compare> {
    public:
        explicit GPUBitonicSort(std::shared_ptr<const GPUSortContext> context)
            : context(std::move(context)) {
            if (!this->context) {
                throw std::invalid_argument("GPUBitonicSort requires a GPU sort context");
            }
        }

        using MergeSort<T, Compare>::perform_sort;

        void perform_sort(std::span<T> values, Compare /*unused*/) override {
            if (values.size() < 2) {
                return;
            }

            const std::uint32_t padded_count = GPUSortContext::checked_size(std::bit_ceil(values.size()));
            graphics::GraphicsDevice& device = context->device();
            const auto buffer = device.allocate_shared(std::size_t{padded_count} * sizeof(T));

            GPUSortContext::upload<T>(values, *buffer, 0);
            const std::vector<T> padding(
                padded_count - values.size(), padding_value(), std::allocator<T>{});
            GPUSortContext::upload<T>(padding, *buffer, values.size());

            const graphics::GraphicsKernel& global_kernel = context->kernel<T>("bitonic_step");
            const graphics::GraphicsKernel& local_kernel = context->kernel<T>("bitonic_local");
            const std::array<graphics::GraphicsBuffer*, 1> buffers{buffer.get()};
            const auto commands = device.begin_commands();

            const auto group_size = static_cast<std::uint32_t>(std::bit_floor(std::min<std::size_t>(
                {local_kernel.max_threads_per_group(), MAX_LOCAL_GROUP, padded_count / 2})));
            const std::uint32_t block_size = group_size * 2;
            const graphics::GridSize pairs{.width = padded_count / 2};
            const graphics::GridSize group{.width = group_size};

            dispatch_pass(*commands, local_kernel, buffers, 0, 0, padded_count, pairs, group);

            for (std::uint32_t stage = block_size * 2; stage <= padded_count; stage <<= 1U) {
                for (std::uint32_t step = stage >> 1U; step >= block_size; step >>= 1U) {
                    dispatch_pass(*commands, global_kernel, buffers, step, stage, padded_count, pairs);
                }

                dispatch_pass(*commands, local_kernel, buffers, 0, stage, padded_count, pairs, group);
            }

            commands->submit_list();
            commands->wait_completed();

            GPUSortContext::download<T>(*buffer, values);
        }

    private:
        static constexpr std::size_t MAX_LOCAL_GROUP = 1024;

        std::shared_ptr<const GPUSortContext> context;

        static GPUSortParameters parameters_for(
            const std::uint32_t step,
            const std::uint32_t stage,
            const std::uint32_t count
        ) {
            return GPUSortParameters{
                .first = step,
                .second = stage,
                .count = count,
                .descending = sorts_descending<Compare, T> ? 1U : 0U,
            };
        }

        static void dispatch_pass(
            graphics::GraphicsCommandList& commands,
            const graphics::GraphicsKernel& kernel,
            const std::span<graphics::GraphicsBuffer* const> buffers,
            const std::uint32_t step,
            const std::uint32_t stage,
            const std::uint32_t count,
            const graphics::GridSize &threads
        ) {
            const GPUSortParameters parameters = parameters_for(step, stage, count);
            commands.dispatch_work(
                kernel,
                buffers,
                std::as_bytes(std::span<const GPUSortParameters, 1>(&parameters, 1)),
                threads);
        }

        static void dispatch_pass(
            graphics::GraphicsCommandList& commands,
            const graphics::GraphicsKernel& kernel,
            const std::span<graphics::GraphicsBuffer* const> buffers,
            const std::uint32_t step,
            const std::uint32_t stage,
            const std::uint32_t count,
            const graphics::GridSize threads,
            const graphics::GridSize group
        ) {
            const GPUSortParameters parameters = parameters_for(step, stage, count);
            commands.dispatch_work(
                kernel,
                buffers,
                std::as_bytes(std::span<const GPUSortParameters, 1>(&parameters, 1)),
                threads,
                group);
        }

        static constexpr T padding_value() {
            if constexpr (std::numeric_limits<T>::has_infinity) {
                return sorts_descending<Compare, T> ? -std::numeric_limits<T>::infinity()
                                                    : std::numeric_limits<T>::infinity();
            } else {
                return sorts_descending<Compare, T> ? std::numeric_limits<T>::lowest()
                                                    : std::numeric_limits<T>::max();
            }
        }
    };
}  // namespace multithreading::parallel_computing::merge_sort
