#include <application/computing_scenarios/merge_sort/SortInputs.h>

#include <algorithm>
#include <limits>
#include <random>

namespace multithreading::computing_scenarios::merge_sort {

    SortInputs::SortInputs(const std::vector<std::size_t>& sizes, const std::uint32_t seed) {
        std::mt19937 generator(seed);
        std::uniform_int_distribution<int> distribution(
            std::numeric_limits<int>::min(),
            std::numeric_limits<int>::max()
        );

        for (const std::size_t size : sizes) {
            std::vector<int> values(size);
            std::ranges::generate(values, [&]() {
                return distribution(generator);
            });
            inputs.insert_or_assign(size, std::move(values));
        }
    }

    const std::vector<int>& SortInputs::of_size(const std::size_t size) const {
        return inputs.at(size);
    }
}  // namespace multithreading::computing_scenarios::merge_sort
