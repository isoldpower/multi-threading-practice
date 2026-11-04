#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace multithreading::computing_scenarios::merge_sort {

    constexpr std::size_t SMALL_INPUT_SIZE = 100'000;
    constexpr std::size_t MEDIUM_INPUT_SIZE = 1'000'000;
    constexpr std::size_t LARGE_INPUT_SIZE = 10'000'000;
    constexpr std::uint32_t INPUT_SEED = 2026;

    [[nodiscard]] std::vector<std::size_t> parallel_thread_counts();
}  // namespace multithreading::computing_scenarios::merge_sort
