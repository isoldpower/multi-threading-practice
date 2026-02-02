#pragma once

#include <optional>


namespace multithreading::structures::linked_list {

    template <typename T>
    struct LinkedListNode {
    protected:
        T node_value;

        explicit LinkedListNode(T value)
            : node_value(value)
        {}
    };

    template <typename T>
    class LinkedList {
    public:
        LinkedList() = default;
        virtual ~LinkedList() = default;

        LinkedList(const LinkedList&) = delete;
        LinkedList& operator=(const LinkedList&) = delete;
        LinkedList(LinkedList&&) = delete;
        LinkedList& operator=(LinkedList&&) = delete;

        virtual void push_front(T item) = 0;
        virtual void push_back(T item) = 0;
        virtual bool push_at(T item, size_t index) = 0;

        virtual std::optional<T> pop_front() = 0;
        virtual std::optional<T> pop_back() = 0;
        virtual std::optional<T> pop_at(size_t index) = 0;

        virtual bool empty() = 0;
        virtual size_t size() = 0;
        virtual bool contains(T value) = 0;
    };
} // namespace multithreading::structures::linked_list