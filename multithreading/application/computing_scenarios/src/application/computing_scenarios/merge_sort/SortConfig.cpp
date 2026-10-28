#include <application/computing_scenarios/merge_sort/SortConfig.h>

#include <algorithm>
#include <thread>

namespace multithreading::computing_scenarios::merge_sort {

    std::vector<std::size_t> parallel_thread_counts() {
        const std::size_t hardware_threads =
            std::max<std::size_t>(2, std::thread::hardware_concurrency());

        std::vector<std::size_t> counts{};
        for (std::size_t count = 2; count < hardware_threads; count *= 2) {
            counts.push_back(count);
        }
        counts.push_back(hardware_threads);

        return counts;
    }
}  // namespace multithreading::computing_scenarios::merge_sort
