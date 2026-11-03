#pragma once

#include <parallel_computing/merge_sort/GPUSortContext.h>
#include <parallel_computing/merge_sort/MergeSort.h>

#include <graphics/GraphicsBuffer.h>
#include <graphics/GraphicsCommandList.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <stdexcept>
#include <utility>

namespace multithreading::parallel_computing::merge_sort {

    template <typename T, typename Compare = std::less<T>>
        requires GPUSortable<T> && GPUSortOrder<Compare, T>
    class GPUMergeSort final : public MergeSort<T, Compare> {
    public:
        explicit GPUMergeSort(std::shared_ptr<const GPUSortContext> context)
            : context(std::move(context)) {
            if (!this->context) {
                throw std::invalid_argument("GPUMergeSort requires a GPU sort context");
            }
        }

        using MergeSort<T, Compare>::perform_sort;

        void perform_sort(std::span<T> values, Compare /*unused*/) override {
            if (values.size() < 2) {
                return;
            }

            const std::uint32_t count = GPUSortContext::checked_size(values.size());
            graphics::GraphicsDevice& device = context->device();
            auto source = device.allocate_shared(values.size_bytes());
            auto destination = device.allocate_shared(values.size_bytes());

            GPUSortContext::upload<T>(values, *source, 0);

            const graphics::GraphicsKernel& kernel = context->kernel<T>("merge_pass");
            const auto commands = device.begin_commands();

            for (std::uint32_t width = 1; width < count; width <<= 1U) {
                const GPUSortParameters parameters{
                    .first = width,
                    .second = 0,
                    .count = count,
                    .descending = sorts_descending<Compare, T> ? 1U : 0U,
                };
                const std::array<graphics::GraphicsBuffer*, 2> buffers{source.get(), destination.get()};
                commands->dispatch_work(
                    kernel,
                    buffers,
                    std::as_bytes(std::span<const GPUSortParameters, 1>(&parameters, 1)),
                    graphics::GridSize{.width = count});

                std::swap(source, destination);
            }

            commands->submit_list();
            commands->wait_completed();

            GPUSortContext::download<T>(*source, values);
        }

    private:
        std::shared_ptr<const GPUSortContext> context;
    };
}  // namespace multithreading::parallel_computing::merge_sort
