#include "../../include/mcmp/LinkedListMCMPBenchmark.h"

#include <multithreading/structures/include/linked_list/FGLockLinkedList.h>
#include <thread>
#include <cstdint>


namespace executables::benchmarks::mcmp {

    constexpr size_t MAX_CONSUME_RETRIES = 100;

    LinkedListMCMPBenchmark::LinkedListMCMPBenchmark(
        const std::shared_ptr<multithreading::structures::linked_list::LinkedList<int>> &list
    )
        : LinkedListBenchmark(list)
    {}

    void LinkedListMCMPBenchmark::producer_routine(const size_t threadSize) {
        for (size_t j = 0; j < threadSize; j++) {
            benchmark_list->push_front(static_cast<int>(j));
        }
    }

    void LinkedListMCMPBenchmark::consumer_routine(const size_t threadSize) {
        for (size_t j = 0; j < threadSize; j++) {
            size_t retries = 0;
            while (!benchmark_list->pop_front().has_value()) {
                if (++retries > MAX_CONSUME_RETRIES) {
                    std::this_thread::yield();
                    retries = 0;
                }
            }
        }
    }
} // namespace executables::benchmarks::mcmp