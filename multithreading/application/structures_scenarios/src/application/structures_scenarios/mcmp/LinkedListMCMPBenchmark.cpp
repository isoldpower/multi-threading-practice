#include <application/structures_scenarios/Backoff.h>
#include <application/structures_scenarios/mcmp/LinkedListMCMPBenchmark.h>

namespace multithreading::structures_scenarios::mcmp {

    LinkedListMCMPBenchmark::LinkedListMCMPBenchmark(
        const std::shared_ptr<structures::linked_list::LinkedList<int>>& list)
        : LinkedListBenchmark(list) {
    }

    void LinkedListMCMPBenchmark::producer_routine(const size_t threadSize) {
        for (size_t j = 0; j < threadSize; j++) {
            list()->push_front(static_cast<int>(j));
        }
    }

    void LinkedListMCMPBenchmark::consumer_routine(const size_t threadSize) {
        for (size_t j = 0; j < threadSize; j++) {
            retry_with_backoff([&] {
                return list()->pop_front().has_value();
            });
        }
    }
}  // namespace multithreading::structures_scenarios::mcmp
