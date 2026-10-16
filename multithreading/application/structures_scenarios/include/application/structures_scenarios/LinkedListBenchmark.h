#pragma once

#include <structures/linked_list/LinkedList.h>

#include <memory>

namespace multithreading::structures_scenarios {

    class LinkedListBenchmark {
    private:
        std::shared_ptr<multithreading::structures::linked_list::LinkedList<int>> benchmark_list;

    protected:
        explicit LinkedListBenchmark(
            const std::shared_ptr<multithreading::structures::linked_list::LinkedList<int>>& list
        );

        [[nodiscard]] const std::shared_ptr<
            multithreading::structures::linked_list::LinkedList<int>>&
        list() const noexcept {
            return benchmark_list;
        }
    };
}  // namespace multithreading::structures_scenarios