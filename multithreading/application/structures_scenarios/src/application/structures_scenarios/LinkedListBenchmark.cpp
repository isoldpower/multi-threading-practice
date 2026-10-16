#include <application/structures_scenarios/LinkedListBenchmark.h>

namespace multithreading::structures_scenarios {

    LinkedListBenchmark::LinkedListBenchmark(
        const std::shared_ptr<multithreading::structures::linked_list::LinkedList<int>>& list
    )
        : benchmark_list(list) {
    }
}  // namespace multithreading::structures_scenarios
