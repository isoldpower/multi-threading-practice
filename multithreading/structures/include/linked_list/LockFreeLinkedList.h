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

        explicit LockFreeNode(T&& value)
            : LinkedListNode<T>(std::move(value))
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

        [[nodiscard]] const T& peek() const {
            return this->node_value;
        }

        [[nodiscard]] T&& value() {
            return std::move(this->node_value);
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
                // next_node was mark(nullptr) = 0x1, we just physically unlink it by setting to nullptr.
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
            while (true) { // Outer restart loop
                size_t iterator = 0;
                LockFreeNode<T>* previous_node = head;
                LockFreeNode<T>* current_node = LockFreeNode<T>::unmark_node(
                    head->next_node.load(std::memory_order_acquire)
                );

                bool conflict = false;

                while (iterator < index) {
                    if (current_node == nullptr) return nullptr;

                    // Load next and check for logical deletion
                    LockFreeNode<T>* next_node = current_node->next_node.load(std::memory_order_acquire);
                    LockFreeNode<T>* unmarked_next = LockFreeNode<T>::unmark_node(next_node);

                    if (LockFreeNode<T>::is_marked(next_node)) {
                        // Try to physically unlink the deleted current_node
                        if (previous_node->next_node.compare_exchange_weak(
                            current_node, // We expect unmarked current_node
                            unmarked_next,
                            std::memory_order_release,
                            std::memory_order_acquire
                        )) {
                            epoch_reclamation.retire_reference(current_node);
                            current_node = unmarked_next;
                            // Do not increment iterator; we just removed a "ghost" node
                            continue;
                        } else {
                            // previous_node changed (likely deleted). Restart from head.
                            conflict = true;
                            break;
                        }
                    }

                    previous_node = current_node;
                    current_node = unmarked_next;
                    iterator++;
                }

                if (!conflict) return previous_node;
            }
        }

        LockFreeNode<T>* traverse_to_second_to_last() {
            while (true) { // Restart loop
                LockFreeNode<T>* previous_node = head;
                LockFreeNode<T>* current_node = LockFreeNode<T>::unmark_node(
                    head->next_node.load(std::memory_order_acquire)
                );

                bool conflict_detected = false;

                while (current_node != nullptr) {
                    LockFreeNode<T>* next_node = current_node->next_node.load(std::memory_order_acquire);
                    LockFreeNode<T>* unmarked_next = LockFreeNode<T>::unmark_node(next_node);

                    if (LockFreeNode<T>::is_marked(next_node)) {
                        // Try to physically unlink 'current_node'
                        if (previous_node->next_node.compare_exchange_weak(
                            current_node,
                            unmarked_next,
                            std::memory_order_release,
                            std::memory_order_acquire
                        )) {
                            // Success: retire and update current_node to keep moving forward
                            epoch_reclamation.retire_reference(current_node);
                            current_node = unmarked_next;
                            continue;
                        } else {
                            // Failure: 'previous_node' was modified or deleted.
                            // Break to the outer loop to restart from head.
                            conflict_detected = true;
                            break;
                        }
                    }

                    if (unmarked_next == nullptr) {
                        return previous_node; // Success: reached second-to-last
                    }

                    previous_node = current_node;
                    current_node = unmarked_next;
                }

                if (!conflict_detected) {
                    return previous_node; // Reached end of list (empty list case)
                }
            }
        }

        LockFreeNode<T>* find_actual_tail() {
            LockFreeNode<T>* previous_node = head;
            LockFreeNode<T>* current_node = LockFreeNode<T>::unmark_node(
                head->next_node.load(std::memory_order_acquire));

            while (current_node != nullptr) {
                LockFreeNode<T>* next_node = current_node->next_node.load(std::memory_order_acquire);

                if (LockFreeNode<T>::is_marked(next_node)) {
                    // Try to help physically delete the node if it is logically deleted.
                    LockFreeNode<T>* unmarked_next = LockFreeNode<T>::unmark_node(next_node);

                    if (previous_node->next_node.compare_exchange_weak(
                        current_node,
                        unmarked_next,
                        std::memory_order_release,
                        std::memory_order_acquire
                    )) {
                        // Retire the node and move forward with next node that was unmarked for safety.
                        epoch_reclamation.retire_reference(current_node);
                        current_node = unmarked_next;
                    } else {
                        // Failed to help advance the node meaning it somehow changed without
                        // our control. For extra safety restart from head.
                        previous_node = head;
                        current_node = LockFreeNode<T>::unmark_node(
                            head->next_node.load(std::memory_order_acquire));
                    }
                } else {
                    // The node is not deleted. Check if it is the last node and move forward if
                    // it is not last.
                    if (next_node == nullptr) {
                        return current_node;
                    }

                    previous_node = current_node;
                    current_node = next_node;
                }
            }

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
            LockFreeNode<T>* new_node = new LockFreeNode<T>(std::move(item));

            while (true) {
                // Get the state of the current head and unmark it to point to a valid address.
                // If the first node is marked then help it advance and retry.
                LockFreeNode<T>* current_head = head->next_node.load(std::memory_order_acquire);
                if (LockFreeNode<T>::is_marked(current_head)) {
                    this->try_help_advance(head, current_head);
                    continue;
                }

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
            LockFreeNode<T>* new_node = new LockFreeNode<T>(std::move(item));

            while (true) {
                LockFreeNode<T>* current_tail = tail.load(std::memory_order_acquire);
                LockFreeNode<T>* after_tail = current_tail->next_node.load(std::memory_order_acquire);

                if (current_tail != tail.load(std::memory_order_acquire)) {
                    // The tail changed between the checks, state is inconsistent.
                    // Retry in next operation.
                    continue;
                }

                if (LockFreeNode<T>::is_marked(after_tail)) {
                    LockFreeNode<T>* unmarked_after = LockFreeNode<T>::unmark_node(after_tail);
                    if (unmarked_after != nullptr) {
                        // Tail lags behind a non-deleted node so we need to help it advance
                        // to fit the current state of the list.
                        tail.compare_exchange_weak(
                            current_tail,
                            unmarked_after,
                            std::memory_order_release,
                            std::memory_order_relaxed);
                    } else {
                        // Tail is lost somewhere hanging 'after' the list due to stale states and possible
                        // concurrent pop_back() calls. Find the actual tail position from scratch.
                        LockFreeNode<T>* actual_tail = this->find_actual_tail();
                        tail.compare_exchange_weak(
                            current_tail,
                            actual_tail,
                            std::memory_order_release,
                            std::memory_order_relaxed);
                    }

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

        ssize_t push_at(const size_t index, T item) {
            utilities::performance::EpochGuard<LockFreeNode<T>> epoch_guard(&epoch_reclamation);
            LockFreeNode<T>* new_node = new LockFreeNode<T>(std::move(item));

            while (true) {
                LockFreeNode<T>* current_node = traverse_to(index);
                // If we are out-of-bounds in traverse then simply delete the new node and return.
                if (current_node == nullptr) {
                    delete new_node;
                    return -1;
                }

                // Traverse successful, try insert the node between current and current->next_node.
                LockFreeNode<T>* next_node = current_node->next_node.load(std::memory_order_acquire);
                if (LockFreeNode<T>::is_marked(next_node)) {
                    // If the node is marked then we need to help it advance and retry.
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
                if (!unmarked_last->next_node.compare_exchange_weak(
                    after_last,
                    marked_after,
                    std::memory_order_release,
                    std::memory_order_acquire
                )) {
                    continue;
                }

                T node_value = unmarked_last->value();
                // At this point the logical deletion with marking was successful. Now we need
                // to physically delete the node from the list AND from memory.
                if (before_last->next_node.compare_exchange_weak(
                    unmarked_last,
                    after_last,
                    std::memory_order_release,
                    std::memory_order_relaxed
                )) {
                    epoch_reclamation.retire_reference(unmarked_last);
                }

                // We ignore all the potential CAS fails because after the logical delete
                // succeeded our tail and before_last are already in a safe state. The potential
                // fails only mean that other threads may have helped advancing the state and
                // retiring the node.
                LockFreeNode<T>* new_tail = (after_last != nullptr) ? after_last : before_last;
                tail.compare_exchange_weak(
                    unmarked_last,
                    new_tail,
                    std::memory_order_release,
                    std::memory_order_relaxed);

                return std::optional<T>(std::move(node_value));
            }
        }

        std::optional<T> pop_at(size_t index) {
            utilities::performance::EpochGuard<LockFreeNode<T>> epoch_guard(&epoch_reclamation);

            while (true) {
                // Try to traverse to the node that was requested to pop. Return if we reached
                // the end of the list without encountering the requested node.
                LockFreeNode<T>* before_node = this->traverse_to(index);
                if (before_node == nullptr) {
                    return std::nullopt;
                }

                // Traverse returns the node before the one requested so we access the referenced
                // node here by accessing the next after returned one.
                LockFreeNode<T>* referenced_node = before_node->next_node.load(std::memory_order_acquire);
                if (referenced_node == nullptr) {
                    return std::nullopt;
                } else if (LockFreeNode<T>::is_marked(referenced_node)) {
                    // If referenced_node has the mark bit, it means before_node is logically deleted.
                    // We cannot use a deleted before_node so we restart the traversal.
                    // NOTE: Probably this->traverse_to() already checks for node being deleted,
                    // but we do it for extra safety here.
                    continue;
                }

                LockFreeNode<T>* unmarked_referenced = LockFreeNode<T>::unmark_node(referenced_node);
                LockFreeNode<T>* next_node = unmarked_referenced->next_node.load(std::memory_order_acquire);

                // If next_node is marked, our target node is logically deleted so we help it
                // advance and retry in the next iteration.
                if (LockFreeNode<T>::is_marked(next_node)) {
                    // We must physically unlink it right here to prevent an infinite loop as
                    // traverse_to() doesn't reach this node and never helps it advance.
                    LockFreeNode<T>* unmarked_next = LockFreeNode<T>::unmark_node(next_node);
                    if (before_node->next_node.compare_exchange_weak(
                        unmarked_referenced,
                        unmarked_next,
                        std::memory_order_release,
                        std::memory_order_relaxed
                    )) {
                        epoch_reclamation.retire_reference(unmarked_referenced);
                    }

                    // Don't care about the result of helping to advance just skip to next iteration.
                    continue;
                }

                // At this point we are sure screened states of the nodes are fine so we
                // can move to the CAS operations with trying to logically delete first.
                if (unmarked_referenced->next_node.compare_exchange_weak(
                    next_node,
                    LockFreeNode<T>::mark_node(next_node),
                    std::memory_order_release,
                    std::memory_order_acquire
                )) {
                    // Logical delete succeeded, and we can move the value from the node and
                    // proceed to physical delete.
                    T node_value = unmarked_referenced->value();
                    if (before_node->next_node.compare_exchange_weak(
                        unmarked_referenced,
                        next_node,
                        std::memory_order_release,
                        std::memory_order_relaxed
                    )) {
                        epoch_reclamation.retire_reference(unmarked_referenced);
                    }

                    // Try to sync the tail if we end up at the end of the list. Ignore
                    // potential fail as it will be handled in '_back()' operations.
                    if (next_node == nullptr) {
                        tail.compare_exchange_weak(
                            unmarked_referenced,
                            before_node,
                            std::memory_order_release,
                            std::memory_order_relaxed);
                    }

                    return std::optional<T>(std::move(node_value));
                }

                // Logical deletion CAS failed so we retry in the next iteration.
                continue;
            }
        }

        bool empty() {
            return head->next_node.load(std::memory_order_acquire) == nullptr;
        }

        size_t size() {
            utilities::performance::EpochGuard<LockFreeNode<T>> epoch_guard(&epoch_reclamation);
            size_t iterator = 0;

            // Unmark first real node to start the traverse process safely.
            LockFreeNode<T>* current_node = LockFreeNode<T>::unmark_node(
                head->next_node.load(std::memory_order_acquire));

            while (current_node != nullptr) {
                LockFreeNode<T>* next_node = current_node->next_node.load(std::memory_order_acquire);

                // If the next node is logically deleted then we are not increasing the iterator
                // because the node is not logically present.
                if (LockFreeNode<T>::is_marked(next_node)) {
                    current_node = LockFreeNode<T>::unmark_node(next_node);
                    continue;
                } else {
                    iterator++;
                    current_node = next_node;
                }
            }

            return iterator;
        }

        bool contains(const T& value, std::function<bool(const T&, const T&)> equaliser) {
            utilities::performance::EpochGuard<LockFreeNode<T>> epoch_guard(&epoch_reclamation);

            // Unmark first real node to start the traverse process safely.
            LockFreeNode<T>* current_node = LockFreeNode<T>::unmark_node(
                head->next_node.load(std::memory_order_acquire));

            while (current_node != nullptr) {
                LockFreeNode<T>* next_node = current_node->next_node.load(std::memory_order_acquire);

                if (LockFreeNode<T>::is_marked(next_node)) {
                    // Skip the node if it is logically deleted.
                    current_node = LockFreeNode<T>::unmark_node(next_node);
                } else {
                    // The node is present and not logically deleted. Check its value for being
                    // equal to the searched one.
                    if (equaliser(current_node->peek(), value)) {
                        return true;
                    }

                    // Move forward if the value is not equal.
                    current_node = next_node;
                }
            }

            return false;
        }

        LinkedListNode<T>* find(const T& value, std::function<bool(const T&, const T&)> equaliser) {
            utilities::performance::EpochGuard<LockFreeNode<T>> epoch_guard(&epoch_reclamation);

            // Unmark first real node to start the traverse process safely.
            LockFreeNode<T>* current_node = LockFreeNode<T>::unmark_node(
                head->next_node.load(std::memory_order_acquire));

            while (current_node != nullptr) {
                LockFreeNode<T>* next_node = current_node->next_node.load(std::memory_order_acquire);

                if (LockFreeNode<T>::is_marked(next_node)) {
                    // Skip the node if it is logically deleted.
                    current_node = LockFreeNode<T>::unmark_node(next_node);
                } else {
                    // The node is present and not logically deleted. Check its value for being
                    // equal to the searched one.
                    if (equaliser(current_node->peek(), value)) {
                        return current_node;
                    }

                    // Move forward if the value is not equal.
                    current_node = next_node;
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

        void push_front(T&& item) override {
            impl->push_front(std::move(item));
        }
        void push_back(T&& item) override {
            impl->push_back(std::move(item));
        }
        bool push_at(const size_t index, T&& item) override {
            const ssize_t operation_result = impl->push_at(index, std::move(item));

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
        bool contains(const T& value) override {
            const auto equaliser = [](const T& first, const T& second) {
                return first == second;
            };

            return impl->contains(value, equaliser);
        }
        bool contains(
            const T& value,
            std::function<bool(const T&, const T&)> equaliser
        ) override {
            return impl->contains(value, equaliser);
        }
    };
} // namespace multithreading::structures::linked_list