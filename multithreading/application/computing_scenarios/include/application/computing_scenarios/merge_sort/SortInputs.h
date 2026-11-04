#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>

namespace multithreading::computing_scenarios::merge_sort {

    class SortInputs {
    private:
        std::map<std::size_t, std::vector<int>> inputs;

    public:
        SortInputs(const std::vector<std::size_t>& sizes, std::uint32_t seed);

        [[nodiscard]] const std::vector<int>& of_size(std::size_t size) const;
    };
}  // namespace multithreading::computing_scenarios::merge_sort
