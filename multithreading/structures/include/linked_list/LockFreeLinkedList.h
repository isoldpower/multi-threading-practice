#pragma once

#include <atomic>
#include <cstdint>
#include <multithreading/utilities/include/performance/EpochGuard.h>
#include <multithreading/utilities/include/performance/EpochReclamation.h>

#include "./LinkedList.h"

namespace multithreading::structures::linked_list {

    template <typename T>
    struct alignas(8) LockFreeNode : public LinkedListNode<T> {
    private:
        static constexpr uintptr_t MARK_BIT = 0x1;
    public:
        std::atomic<LockFreeNode<T>*> next_node;

        LockFreeNode()
            : LinkedListNode<T>(T{})
            , next_node(nullptr)
        {}

        explicit LockFreeNode(T value)
            : LinkedListNode<T>(value)
            , next_node(nullptr)
        {}

        static bool is_marked(LockFreeNode<T>* node) {
            return (reinterpret_cast<uintptr_t>(node) & MARK_BIT) != 0;
        }

        static LockFreeNode<T>* unmark_node(LockFreeNode<T>* node) {
            return reinterpret_cast<LockFreeNode<T>*>(
                reinterpret_cast<uintptr_t>(node) & ~MARK_BIT
            );
        }

        static LockFreeNode<T>* mark_node(LockFreeNode<T>* node) {
            return reinterpret_cast<LockFreeNode<T>*>(
                reinterpret_cast<uintptr_t>(node) | MARK_BIT
            );
        }

        [[nodiscard]] T value() {
            return this->node_value;
        }
    };

    template <typename T>
    class LockFreeLinkedListImpl {
    private:
        LockFreeNode<T>* head;
        std::atomic<LockFreeNode<T>*> tail;

        utilities::performance::EpochReclamation<LockFreeNode<T>> epoch_reclamation;

        void try_help_advance(LockFreeNode<T>* node, LockFreeNode<T>* next_node) {
            if (node == nullptr) {
                return;
            }

            // Unmark the nodes for safety reasons.
            LockFreeNode<T>* unmarked_next = LockFreeNode<T>::unmark_node(next_node);
            if (unmarked_next == nullptr) {
                // next_node was mark(nullptr) = 0x1, just physically unlink by setting to nullptr
                node->next_node.compare_exchange_weak(
                    next_node,
                    nullptr,
                    std::memory_order_release,
                    std::memory_order_acquire
                );
                return;
            }

            LockFreeNode<T>* after_next = unmarked_next->next_node.load(std::memory_order_acquire);
            LockFreeNode<T>* unmarked_after_next = LockFreeNode<T>::unmark_node(after_next);

            // Help advance (physical delete) the next_node, then skip.
            if (node->next_node.compare_exchange_weak(
                next_node,
                unmarked_after_next,
                std::memory_order_release,
                std::memory_order_acquire
            )) {
                epoch_reclamation.retire_reference(unmarked_next);
            }
            // Ignore if we fail.
        }

        LockFreeNode<T>* traverse_to(const size_t index) {
            size_t iterator = 0;
            LockFreeNode<T>* current_node = head;

            // Traverse to the specified index.
            while (iterator < index) {
                LockFreeNode<T>* next_node = current_node->next_node.load(std::memory_order_acquire);
                if (next_node == nullptr) {
                    // Reached the end of the list and did not reach the passed index.
                    // Argument is incorrect, return.
                    return nullptr;
                }

                if (LockFreeNode<T>::is_marked(next_node)) {
                    this->try_help_advance(current_node, next_node);
                    continue;
                } else {
                    // The state is consistent. Increase the iterator to proceed to next.
                    current_node = LockFreeNode<T>::unmark_node(next_node);
                    iterator++;
                }
            }

            return current_node;
        }

        LockFreeNode<T>* traverse_to_second_to_last() {
            LockFreeNode<T>* previous_node = head;
            LockFreeNode<T>* current_node = LockFreeNode<T>::unmark_node(head->next_node.load(std::memory_order_acquire));

            while (current_node != nullptr) {
                LockFreeNode<T>* next_node = current_node->next_node.load(std::memory_order_acquire);
                LockFreeNode<T>* unmarked_next = LockFreeNode<T>::unmark_node(next_node);

                if (LockFreeNode<T>::is_marked(next_node)) {
                    // current_node is being deleted — skip it from previous_node
                    if (previous_node->next_node.compare_exchange_weak(
                        current_node,
                        unmarked_next,
                        std::memory_order_release,
                        std::memory_order_acquire
                    )) {
                        epoch_reclamation.retire_reference(current_node);
                    }
                    // Reload current node from previous node state
                    current_node = LockFreeNode<T>::unmark_node(previous_node->next_node.load(std::memory_order_acquire));
                    continue;
                }

                if (unmarked_next == nullptr) {
                    // current_node is the last node, previous_node is second-to-last
                    return previous_node;
                }

                previous_node = current_node;
                current_node = unmarked_next;
            }

            // Return the traversed node. If the list is empty it will return the head
            // which is a dummy node.
            return previous_node;
        }
    public:
        LockFreeLinkedListImpl()
            : head(new LockFreeNode<T>())
            , tail(head)
            , epoch_reclamation()
        {}

        ~LockFreeLinkedListImpl() {
            while (pop_front().has_value()) {}
            delete head;
        }

        void push_front(T item) {
            utilities::performance::EpochGuard<LockFreeNode<T>> epoch_guard(&epoch_reclamation);
            LockFreeNode<T>* new_node = new LockFreeNode<T>(item);

            while (true) {
                // Get the state of the current head and unmark it to point to a valid address.
                LockFreeNode<T>* current_head = head->next_node.load(std::memory_order_acquire);
                LockFreeNode<T>* unmarked = LockFreeNode<T>::unmark_node(current_head);
                new_node->next_node.store(unmarked, std::memory_order_relaxed);

                // If the head did not change, then we insert the new node with CAS operation.
                // If we did - continue to next iteration until success.
                if (head->next_node.compare_exchange_weak(
                    current_head,
                    new_node,
                    std::memory_order_release,
                    std::memory_order_acquire
                )) {
                    return;
                }
            }
        }

        void push_back(T item) {
            utilities::performance::EpochGuard<LockFreeNode<T>> epoch_guard(&epoch_reclamation);
            LockFreeNode<T>* new_node = new LockFreeNode<T>(item);

            while (true) {
                LockFreeNode<T>* current_tail = tail.load(std::memory_order_acquire);
                LockFreeNode<T>* after_tail = current_tail->next_node.load(std::memory_order_acquire);

                if (current_tail != tail.load(std::memory_order_acquire)) {
                    continue;
                }
                if (LockFreeNode<T>::is_marked(after_tail)) {
                    this->try_help_advance(current_tail, after_tail);
                    continue;
                }

                // Our tail did not change, it is consistent.
                if (after_tail == nullptr) {
                    // The desired behavior, we are NOT in the middle of other thread's write.
                    if (current_tail->next_node.compare_exchange_weak(
                        after_tail,
                        new_node,
                        std::memory_order_release,
                        std::memory_order_acquire
                    )) {
                        // Successful binding of new node to the list. Now only tail
                        // reference update is required.
                        tail.compare_exchange_weak(
                            current_tail,
                            new_node,
                            std::memory_order_release,
                            std::memory_order_relaxed);

                        return;
                    } else {
                        // The update failed. Just proceed to the next iteration to retry.
                        continue;
                    }
                } else {
                    // Other thread is currently writing to the tail but the tail reference
                    // laggs behind. Try helping in advancing the tail to the actual node.
                    tail.compare_exchange_weak(
                        current_tail,
                        after_tail,
                        std::memory_order_release,
                        std::memory_order_relaxed);
                }
            }
        }

        ssize_t push_at(T item, const size_t index) {
            utilities::performance::EpochGuard<LockFreeNode<T>> epoch_guard(&epoch_reclamation);
            LockFreeNode<T>* new_node = new LockFreeNode<T>(item);

            while (true) {
                LockFreeNode<T>* current_node = traverse_to(index);
                // If we are out-of-bounds in traverse then simply delete the new node and return.
                if (current_node == nullptr) {
                    delete new_node;
                    return -1;
                }

                LockFreeNode<T>* next_node = current_node->next_node.load(std::memory_order_acquire);
                // Traverse successful, insert the node between current and current->next_node.
                if (LockFreeNode<T>::is_marked(next_node)) {
                    this->try_help_advance(current_node, next_node);
                    continue;
                } else {
                    LockFreeNode<T>* unmarked_next = LockFreeNode<T>::unmark_node(next_node);
                    new_node->next_node.store(unmarked_next, std::memory_order_relaxed);

                    if (current_node->next_node.compare_exchange_weak(
                        next_node,
                        new_node,
                        std::memory_order_release,
                        std::memory_order_acquire
                    )) {
                        if (unmarked_next == nullptr) {
                            tail.compare_exchange_weak(
                                current_node,
                                new_node,
                                std::memory_order_release,
                                std::memory_order_relaxed);
                        }

                        return 0;
                    } else {
                        // Our current_node changed somewhere in the process.
                        // Retry on next iteration.
                        continue;
                    }
                }
            }
        }

        std::optional<T> pop_front() {
            utilities::performance::EpochGuard<LockFreeNode<T>> epoch_guard(&epoch_reclamation);

            while (true) {
                LockFreeNode<T>* current_head = head->next_node.load(std::memory_order_acquire);
                if (LockFreeNode<T>::is_marked(current_head)) {
                    // The next is marked, help advance the head's next reference and continue
                    // in the next iteration.
                    this->try_help_advance(head, current_head);
                    continue;
                } else if (current_head == nullptr) {
                    return std::nullopt;
                } else {
                    // The next is not marked, move to deletion.
                    LockFreeNode<T>* next_node = current_head->next_node.load(std::memory_order_acquire);
                    if (LockFreeNode<T>::is_marked(next_node)) {
                        LockFreeNode<T>* unmarked_next = LockFreeNode<T>::unmark_node(next_node);
                        if (head->next_node.compare_exchange_weak(
                            current_head,
                            unmarked_next,
                            std::memory_order_release,
                            std::memory_order_acquire
                        )) {
                            epoch_reclamation.retire_reference(current_head);
                        }

                        continue;
                    }

                    LockFreeNode<T>* marked_next = LockFreeNode<T>::mark_node(next_node);
                    if (current_head->next_node.compare_exchange_weak(
                        next_node,
                        marked_next,
                        std::memory_order_release,
                        std::memory_order_acquire
                    )) {
                        // The logical deletion was successful - move to the physical deletion.
                        T node_value = current_head->value();
                        LockFreeNode<T>* unmarked_next = LockFreeNode<T>::unmark_node(next_node);
                        if (head->next_node.compare_exchange_weak(
                            current_head,
                            unmarked_next,
                            std::memory_order_release,
                            std::memory_order_relaxed
                        )) {
                            epoch_reclamation.retire_reference(current_head);
                        }

                        return node_value;
                    } else {
                        // CAS failed, someone updated the head->next. Retry in next iteration.
                        continue;
                    }
                }
            }
        }

        std::optional<T> pop_back() {
            utilities::performance::EpochGuard<LockFreeNode<T>> epoch_guard(&epoch_reclamation);

            while (true) {
                // Traverse to the node before-the-tail.
                LockFreeNode<T>* before_last = this->traverse_to_second_to_last();
                LockFreeNode<T>* last_node = before_last->next_node.load(std::memory_order_acquire);
                if (last_node == nullptr) {
                    if (before_last == head) {
                        return std::nullopt;
                    }

                    continue;
                } else if (LockFreeNode<T>::is_marked(last_node)) {
                    // this->try_help_advance(before_last, last_node);
                    continue;
                }

                // At this point last_node should already be unmarked, but we try to unmark
                // one more time for extra safety.
                LockFreeNode<T>* unmarked_last = LockFreeNode<T>::unmark_node(last_node);
                LockFreeNode<T>* after_last = unmarked_last->next_node.load(std::memory_order_acquire);
                if (LockFreeNode<T>::is_marked(after_last)) {
                    // Someone else is deleting this node
                    this->try_help_advance(before_last, last_node);
                    continue;
                }

                LockFreeNode<T>* marked_after = LockFreeNode<T>::mark_node(after_last);
                T node_value = unmarked_last->value();
                if (!unmarked_last->next_node.compare_exchange_weak(
                    after_last,
                    marked_after,
                    std::memory_order_release,
                    std::memory_order_acquire
                )) {
                    continue;
                }

                // At this point the logical deletion with marking was successful. Now we need
                // to physically delete the node from the list AND from memory.
                if (before_last->next_node.compare_exchange_weak(
                    unmarked_last,
                    nullptr,
                    std::memory_order_release,
                    std::memory_order_relaxed
                )) {
                    epoch_reclamation.retire_reference(unmarked_last);
                }

                // We ignore all the potential CAS fails because after the logical delete
                // succeeded our tail and before_last are already in a safe state. The potential
                // fails only mean that other threads may have helped advancing the state and
                // retiring the node.
                tail.compare_exchange_weak(
                    unmarked_last,
                    before_last,
                    std::memory_order_release,
                    std::memory_order_relaxed);

                return std::optional<T>(node_value);
            }
        }

        std::optional<T> pop_at(size_t index) {
            utilities::performance::EpochGuard<LockFreeNode<T>> epoch_guard(&epoch_reclamation);

            while (true) {
                LockFreeNode<T>* before_node = this->traverse_to(index);
                if (before_node == nullptr) {
                    return std::nullopt;
                }
                LockFreeNode<T>* referenced_node = before_node->next_node.load(std::memory_order_acquire);
                if (referenced_node == nullptr) {
                    return std::nullopt;
                }

                if (LockFreeNode<T>::is_marked(referenced_node)) {
                    // Help advance. Skip to next iteration.
                    this->try_help_advance(before_node, referenced_node);
                    continue;
                } else {
                    LockFreeNode<T>* unmarked_referenced = LockFreeNode<T>::unmark_node(referenced_node);
                    T node_value = unmarked_referenced->value();
                    LockFreeNode<T>* next_node = unmarked_referenced->next_node.load(std::memory_order_acquire);
                    LockFreeNode<T>* next_marked = LockFreeNode<T>::mark_node(next_node);
                    if (LockFreeNode<T>::is_marked(next_node)) {
                        // If the next node is logically deleted - help advance it and retry.
                        this->try_help_advance(before_node, unmarked_referenced);
                        continue;
                    }

                    if (unmarked_referenced->next_node.compare_exchange_weak(
                        next_node,
                        next_marked,
                        std::memory_order_release,
                        std::memory_order_acquire
                    )) {
                        // Successful logical delete. Move to physical delete. The next_node
                        // is unmarked at this point due to the check above.
                        if (before_node->next_node.compare_exchange_weak(
                            unmarked_referenced,
                            next_node,
                            std::memory_order_release,
                            std::memory_order_relaxed
                        )) {
                            epoch_reclamation.retire_reference(unmarked_referenced);
                        }

                        // If we popped the last node - try advance the tail to reference the
                        // previous node.
                        if (next_node == nullptr) {
                            tail.compare_exchange_weak(
                                unmarked_referenced,
                                before_node,
                                std::memory_order_release,
                                std::memory_order_relaxed);
                        }

                        return std::optional<T>(node_value);
                    } else {
                        // CAS update for the logical delete failed. Move to the next iteration.
                        continue;
                    }
                }
            }
        }

        bool empty() {
            return head->next_node.load(std::memory_order_acquire) == nullptr;
        }

        size_t size() {
            utilities::performance::EpochGuard<LockFreeNode<T>> epoch_guard(&epoch_reclamation);

            size_t iterator = 0;
            LockFreeNode<T>* current_node = head->next_node.load(std::memory_order_acquire);

            // Traverse to the end of the list and count the size as we go.
            // Return as soon as we encounter the nullptr (meaning the end of the list)
            while (current_node != nullptr) {
                // No advance-helping. Size() function should be const, without implicitly
                // advancing or changing the nodes.
                if (LockFreeNode<T>::is_marked(current_node)) {
                    LockFreeNode<T>* current_unmarked = LockFreeNode<T>::unmark_node(current_node);
                    current_node = current_unmarked->next_node.load(std::memory_order_acquire);
                    continue;
                }

                iterator++;
                current_node = current_node->next_node.load(std::memory_order_acquire);
            }

            return iterator;
        }

        bool contains(const T& value) {
            utilities::performance::EpochGuard<LockFreeNode<T>> epoch_guard(&epoch_reclamation);

            LockFreeNode<T>* current_node = head->next_node.load(std::memory_order_acquire);

            // Traverse to the end of the list and count the size as we go.
            // Return as soon as we encounter the nullptr (meaning the end of the list)
            while (current_node != nullptr) {
                // No advance-helping. Contains() function should be const, without implicitly
                // advancing or changing the nodes.
                if (LockFreeNode<T>::is_marked(current_node)) {
                    LockFreeNode<T>* current_unmarked = LockFreeNode<T>::unmark_node(current_node);
                    current_node = current_unmarked->next_node.load(std::memory_order_acquire);
                    continue;
                }

                T node_value = current_node->value();
                if (node_value == value) {
                    return true;
                } else {
                    current_node = current_node->next_node.load(std::memory_order_acquire);
                }
            }

            return false;
        }

        LinkedListNode<T>* find(const T& value) {
            utilities::performance::EpochGuard<LockFreeNode<T>> epoch_guard(&epoch_reclamation);

            LockFreeNode<T>* current_node = head->next_node.load(std::memory_order_acquire);

            // Traverse to the end of the list and count the size as we go.
            // Return as soon as we encounter the nullptr (meaning the end of the list)
            while (current_node != nullptr) {
                // No advance-helping. Find() function should be const, without implicitly
                // advancing or changing the nodes.
                if (LockFreeNode<T>::is_marked(current_node)) {
                    LockFreeNode<T>* current_unmarked = LockFreeNode<T>::unmark_node(current_node);
                    current_node = current_unmarked->next_node.load(std::memory_order_acquire);
                    continue;
                }

                T node_value = current_node->value();
                if (node_value == value) {
                    return current_node;
                } else {
                    current_node = current_node->next_node.load(std::memory_order_acquire);
                }
            }

            return nullptr;
        }
    };

    template <typename T>
    class LockFreeLinkedList : public LinkedList<T> {
    private:
        std::unique_ptr<LockFreeLinkedListImpl<T>> impl;
    public:
        LockFreeLinkedList()
            : LinkedList<T>()
            , impl(std::make_unique<LockFreeLinkedListImpl<T>>())
        {}

        ~LockFreeLinkedList() override = default;

        LockFreeLinkedList(const LockFreeLinkedList<T>& other) = delete;
        LockFreeLinkedList<T>& operator=(const LockFreeLinkedList<T>& other) = delete;
        LockFreeLinkedList(LockFreeLinkedList<T>&& other) = delete;
        LockFreeLinkedList<T>& operator=(LockFreeLinkedList<T>&& other) = delete;

        void push_front(T item) override {
            impl->push_front(item);
        }
        void push_back(T item) override {
            impl->push_back(item);
        }
        bool push_at(T item, const size_t index) override {
            const ssize_t operation_result = impl->push_at(item, index);

            return operation_result == 0;
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