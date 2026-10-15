#include <exception>
#include <iostream>

#include "multithreading/structures/include/linked_list/FGLockLinkedList.h"

int main() {
    try {
        multithreading::structures::linked_list::FGLockLinkedList<int> linked_list;

        linked_list.push_front(1);
        linked_list.push_front(2);
        linked_list.push_front(3);

        if (const auto last = linked_list.pop_back(); last.has_value()) {
            std::cout << *last;
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
