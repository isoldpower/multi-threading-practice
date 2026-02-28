#pragma once

#include <memory>
#include <mutex>

#include "./LinkedList.h"

namespace multithreading::structures::linked_list {

    template <typename T>
    struct alignas(64) FGLockNode : public LinkedListNode<T> {
    private:
        std::mutex node_mutex;
        FGLockNode<T>* next_node;
    public:
        explicit FGLockNode(const T& value)
            : LinkedListNode<T>(value)
            , next_node(nullptr)
        {}

        explicit FGLockNode(T&& value)
            : LinkedListNode<T>(std::move(value))
            , next_node(nullptr)
        {}

        FGLockNode()
            : LinkedListNode<T>(T{})
            , next_node(nullptr)
        {}

        [[nodiscard]] T value() {
            return this->node_value;
        }

        [[nodiscard]] FGLockNode<T>* next() {
            return this->next_node;
        }

        void set_next(FGLockNode<T>* node) {
            this->next_node = node;
        }

        void operate() {
            node_mutex.lock();
        }

        void dispose() {
            node_mutex.unlock();
        }
    };

    template <typename T>
    class FGLockLinkedListImpl {
    private:
        FGLockNode<T>* head;
        FGLockNode<T>* tail;

        std::mutex tail_mutex;
        std::mutex count_mutex;
        size_t count {0};

        void increment_count() {
            std::lock_guard<std::mutex> lock(count_mutex);
            ++count;
        }

        void decrement_count() {
            std::lock_guard<std::mutex> lock(count_mutex);
            --count;
        }
    public:
        FGLockLinkedListImpl()
            : head(new FGLockNode<T>())
            , tail(head)
        {}

        ~FGLockLinkedListImpl() {
            while (pop_front().has_value()) {}
            delete head;
        }

        void push_front(T item) {
            FGLockNode<T>* new_node = new FGLockNode<T>(item);

            head->operate();
            FGLockNode<T>* current_head = head->next();
            new_node->set_next(current_head);
            head->set_next(new_node);

            if (current_head == nullptr) {
                std::lock_guard<std::mutex> lock(tail_mutex);
                tail = new_node;
            }

            head->dispose();
            increment_count();
        }

        void push_back(T item) {
            FGLockNode<T>* new_node = new FGLockNode<T>(item);

            // Update last node's contents with internal lock.
            std::scoped_lock<std::mutex> lock(tail_mutex);
            tail->operate();
            tail->set_next(new_node);
            tail->dispose();
            tail = new_node;

            // Increase the current size of the list.
            increment_count();
        }

        LinkedListNode<T>* push_at(size_t index, T item) {
            size_t current_index = 0;
            FGLockNode<T>* iterator_node = head;
            FGLockNode<T>* iterator_next = nullptr;

            // Traverse up the queue to find the element at specified index.
            iterator_node->operate();
            while (current_index < index) {
                // We slide up the list: lock the current element first.
                iterator_next = iterator_node->next();
                if (iterator_next == nullptr) {
                    // If the list has came to its end and we didn't find the desired index,
                    // then we return nullptr. Happens when index > queue size.
                    iterator_node->dispose();
                    return nullptr;
                }

                // Perform the slide up: lock the next element, unlock the current
                // (as it now becomes 'previous').
                iterator_next->operate();
                iterator_node->dispose();
                iterator_node = iterator_next;
                current_index++;
            }

            // Create a new node and update all the references with proper mutex locking.
            FGLockNode<T>* new_node = new FGLockNode<T>(item);
            new_node->set_next(iterator_node->next());
            iterator_node->set_next(new_node);
            // If the element we push is the last node, then update the tail reference.
            if (iterator_node == tail) {
                std::lock_guard<std::mutex> lock(tail_mutex);
                tail = new_node;
            }
            // Update the counter to be up-to-date with actual list size.
            increment_count();
            iterator_node->dispose();
            return new_node;
        }

        std::optional<T> pop_front() {
            head->operate();
            // Access the first real element, bypassing dummy node.
            FGLockNode<T>* current_head = head->next();
            // If the list is empty simply skip the processing.
            if (current_head == nullptr) {
                head->dispose();
                return std::nullopt;
            }

            // Lock the first element to safely delete it from the list.
            current_head->operate();
            head->set_next(current_head->next());
            // If the node deleted was the last one (only one node in the list), then we
            // need to update the tail reference to be up-to-date.
            if (current_head == tail) {
                std::lock_guard<std::mutex> lock(tail_mutex);
                tail = head;
            }
            // Actually delete the node that is now out of the list. Unlock before freeing memory.
            head->dispose();
            T result = current_head->value();
            current_head->dispose();
            delete current_head;

            // Update the counter after successful deletion. Return the snapshot of node's value.
            decrement_count();
            return std::optional<T>(result);
        }

        std::optional<T> pop_back() {
            // Start with locking the tail as we will need it anyway in the future. Early lock
            // prevents deadlocks with push_back().
            std::scoped_lock tail_lock(tail_mutex);
            // Initial state to begin iterating.
            FGLockNode<T>* iterator_node = head;
            FGLockNode<T>* iterator_next =  nullptr;

            iterator_node->operate();
            iterator_next = iterator_node->next();

            // Check if we have anything to pop from the list.
            if (iterator_next == nullptr) {
                // If the list is empty, unlock the dummy.
                iterator_node->dispose();
                return std::nullopt;
            }

            // Find the second-to-last node to update the reference.
            iterator_next->operate();
            while (true) {
                FGLockNode<T>* next = iterator_next->next();
                if (next == nullptr) {
                    break;
                }

                next->operate();
                iterator_node->dispose();
                iterator_node = iterator_next;
                iterator_next = next;
            }

            iterator_node->set_next(nullptr);
            // Delete the last node (iterator_next) and update the pre-last (iterator_node)
            // and tail references.
            tail = iterator_node;
            T result = iterator_next->value();
            iterator_next->dispose();
            iterator_node->dispose();
            delete iterator_next;

            decrement_count();
            return std::optional<T>(result);
        }

        std::optional<T> pop_at(size_t index) {
            // Initial state to begin iterating.
            FGLockNode<T>* iterator_node = head;
            FGLockNode<T>* iterator_next = nullptr;

            iterator_node->operate();
            iterator_next = iterator_node->next();

            // If the list is empty, skip all further processing.
            if (iterator_next == nullptr) {
                iterator_node->dispose();
                return std::nullopt;
            }

            // Find the node at passed index (iterator_next), previous node (iterator_node),
            // and hold the locks on them to safely modify further.
            iterator_next->operate();
            for (size_t i = 0; i < index; i++) {
                FGLockNode<T>* next_node = iterator_next->next();
                if (next_node == nullptr) {
                    iterator_next->dispose();
                    iterator_node->dispose();
                    return std::nullopt;
                }

                next_node->operate();
                iterator_node->dispose();
                iterator_node = iterator_next;
                iterator_next = next_node;
            }

            // Update the reference at previous node. Lock is already on hold after traverse.
            T result = iterator_next->value();
            iterator_node->set_next(iterator_next->next());
            // Safely update the tail reference if required.
            if (iterator_next == tail) {
                std::lock_guard<std::mutex> lock(tail_mutex);
                tail = iterator_node;
            }

            // Release locks, delete the node that is not in the list anymore.
            iterator_node->dispose();
            iterator_next->dispose();
            delete iterator_next;

            decrement_count();
            return std::optional<T>(result);
        }

        size_t size() {
            std::lock_guard<std::mutex> lock(count_mutex);
            return count;
        }

        bool empty() {
            std::lock_guard<std::mutex> lock(count_mutex);
            return count == 0;
        }

        bool contains(T item) {
            // Initial state to begin iterating.
            FGLockNode<T>* iterator_node = head;
            FGLockNode<T>* iterator_next = nullptr;

            iterator_node->operate();
            iterator_next = iterator_node->next();

            // Traverse the list to find the desired node. Return if found.
            while (iterator_next != nullptr) {
                iterator_next->operate();

                if (iterator_next->value() == item) {
                    // If found, free the nodes and return true.
                    iterator_next->dispose();
                    iterator_node->dispose();
                    return true;
                }

                iterator_node->dispose();
                iterator_node = iterator_next;
                iterator_next = iterator_node->next();
            }

            // Free the last node and return false.
            iterator_node->dispose();
            return false;
        }

        LinkedListNode<T>* find(const T& item) {
            // Initial state to begin iterating.
            FGLockNode<T>* iterator_node = head;
            FGLockNode<T>* iterator_next = nullptr;

            iterator_node->operate();
            iterator_next = iterator_node->next();

            // Traverse the list to find the desired node. Return if found.
            while (iterator_next != nullptr) {
                iterator_next->operate();

                if (iterator_next->value() == item) {
                    // If found, free the nodes and return the found node.
                    iterator_next->dispose();
                    iterator_node->dispose();
                    return iterator_next;
                }

                iterator_node->dispose();
                iterator_node = iterator_next;
                iterator_next = iterator_node->next();
            }

            // Free the last node and return false.
            iterator_node->dispose();
            return nullptr;
        }
    };

    template <typename T>
    class FGLockLinkedList final : public LinkedList<T> {
    private:
        std::unique_ptr<FGLockLinkedListImpl<T>> impl;
    public:
        FGLockLinkedList() : impl(std::make_unique<FGLockLinkedListImpl<T>>()) {}
        FGLockLinkedList(const FGLockLinkedList<T> &other) = delete;
        FGLockLinkedList(FGLockLinkedList<T> &&other) = delete;

        FGLockLinkedList<T>& operator=(const FGLockLinkedList<T> &other) = delete;
        FGLockLinkedList<T>& operator=(const FGLockLinkedList<T> &&other) = delete;

        ~FGLockLinkedList() override = default;

        void push_front(T item) override {
            impl->push_front(item);
        }
        void push_back(T item) override {
            impl->push_back(item);
        }
        bool push_at(size_t index, T item) override {
            return impl->push_at(index, item);
        }

        std::optional<T> pop_front() override {
            return impl->pop_front();
        }
        std::optional<T> pop_back() override {
            return impl->pop_back();
        }
        std::optional<T> pop_at(size_t index) override {
            return impl->pop_at(index);
        }

        bool empty() override {
            return impl->empty();
        }

        size_t size() override {
            return impl->size();
        }

        bool contains(T value) override {
            return impl->contains(value);
        }
    };
} // namespace multithreading::structures::linked_list
