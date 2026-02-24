#pragma once

#include <memory>
#include <multithreading/structures/include/linked_list/LinkedList.h>


namespace executables::benchmarks {

    class LinkedListBenchmark {
    protected:
        const std::shared_ptr<multithreading::structures::linked_list::LinkedList<int>> &benchmark_list;

        explicit LinkedListBenchmark(
            const std::shared_ptr<multithreading::structures::linked_list::LinkedList<int>> &list
        );
    };
} // namespace executables::benchmarks