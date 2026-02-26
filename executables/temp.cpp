#include <iostream>

#include "multithreading/structures/include/linked_list/FGLockLinkedList.h"

int main() {
    multithreading::structures::linked_list::FGLockLinkedList<int> linked_list;

    linked_list.push_front(1);
    linked_list.push_front(2);
    linked_list.push_front(3);

    std::cout << linked_list.pop_back().value();
}