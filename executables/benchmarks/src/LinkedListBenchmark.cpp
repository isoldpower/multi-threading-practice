#include "../include/LinkedListBenchmark.h"

namespace executables::benchmarks {

    LinkedListBenchmark::LinkedListBenchmark(
        const std::shared_ptr<multithreading::structures::linked_list::LinkedList<int>> &list
    )
        : benchmark_list(list)
    {}
} // namespace executables::benchmarks
