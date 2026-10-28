#pragma once

#include <structures/unbounded_queue/UnboundedQueue.h>
#include <utilities/performance/AlignedField.h>
#include <utilities/performance/EpochGuard.h>
#include <utilities/performance/EpochReclamation.h>
#include <utilities/performance/Owner.h>
#include <utilities/threads/ItemSignal.h>

#include <atomic>
#include <cstddef>
#include <memory>
#include <optional>

namespace multithreading::structures::unbounded_queue {

    template <typename T>
    // A whole cache line per node is deliberate, so the packing check does not apply
    // NOLINTNEXTLINE(altera-struct-pack-align)
    struct alignas(utilities::performance::CACHE_LINE_SIZE) LockFreeNode {
    private:
        T value{};
        std::atomic<LockFreeNode*> nextNode{nullptr};

    public:
        explicit LockFreeNode(const T& value)
            requires std::is_copy_constructible_v<T>
            : value(value) {
        }

        explicit LockFreeNode(T&& value)
            : value(std::move(value)) {
        }

        LockFreeNode() = default;

        [[nodiscard]] T&& get() {
            return std::move(value);
        }

        [[nodiscard]] LockFreeNode* next() const {
            return nextNode.load(std::memory_order_acquire);
        }

        [[nodiscard]] std::atomic<LockFreeNode*>& nextAtomic() {
            return nextNode;
        }

        void setNext(LockFreeNode* next) {
            this->nextNode.store(next, std::memory_order_release);
        }
    };

    template <typename T>
    class LockFreeUnboundedQueueImpl {
    private:
        utilities::performance::AlignedField<std::atomic<LockFreeNode<T>*>> head;
        utilities::performance::AlignedField<std::atomic<LockFreeNode<T>*>> tail;
        utilities::performance::EpochReclamation<LockFreeNode<T>> epoch_reclamation;
        utilities::threads::ItemSignal items_signal;

        static gsl::owner<LockFreeNode<T>*> create_dummy() {
            return new LockFreeNode<T>();
        }

        static gsl::owner<LockFreeNode<T>*> adopt(LockFreeNode<T>* node) noexcept {
            return node;
        }

        void enqueue_node(std::unique_ptr<LockFreeNode<T>> newNode) {
            const utilities::performance::EpochGuard<LockFreeNode<T>> guard(&epoch_reclamation);

            // A failed attempt means another thread made progress, so retrying until this one
            // succeeds is what keeps the queue lock-free
            while (true) {
                LockFreeNode<T>* last = tail.get().load(std::memory_order_acquire);
                LockFreeNode<T>* next = last->next();

                // If the tail updated between reads, then we want to
                // retry reaching real tail in the next iteration
                if (last == tail.get().load(std::memory_order_acquire)) {
                    if (next == nullptr) {
                        LockFreeNode<T>* expected = nullptr;
                        if (last->nextAtomic().compare_exchange_weak(
                                expected,
                                newNode.get(),
                                std::memory_order_release,
                                std::memory_order_acquire
                            )) {
                            // Write successful: the list owns the node from here on. Try to
                            // update the tail.
                            LockFreeNode<T>* linked = newNode.release();
                            tail.get().compare_exchange_weak(
                                last,
                                linked,
                                std::memory_order_release,
                                std::memory_order_acquire
                            );
                            return;
                        }
                    } else {
                        // At this point our tail didn't change, but the reference to next
                        // got updated on tail. So we want to help our tail to be up-to-date.
                        tail.get().compare_exchange_weak(
                            last,
                            next,
                            std::memory_order_release,
                            std::memory_order_acquire
                        );
                    }
                }
            }
        }

        std::optional<T> dequeue_node() {
            const utilities::performance::EpochGuard<LockFreeNode<T>> guard(&epoch_reclamation);

            while (true) {
                LockFreeNode<T>* first = head.get().load(std::memory_order_acquire);
                LockFreeNode<T>* last = tail.get().load(std::memory_order_acquire);
                LockFreeNode<T>* firstValuable = first->next();

                // Check if data is valid and didn't change between reads. If it did - just continue
                // and redo the action in the next iteration.
                if (first == head.get().load(std::memory_order_acquire)) {
                    if (first == last) {
                        if (firstValuable == nullptr) {
                            // If our head equals to tail - it means that the queue is empty
                            // and they reference the dummy node.
                            return std::nullopt;
                        }
                        // But if the firstValuable node is not nullptr - it means that tail
                        // wasn't updated, so its lagging behind. Help advance it
                        tail.get().compare_exchange_weak(
                            last,
                            firstValuable,
                            std::memory_order_release,
                            std::memory_order_acquire
                        );
                        // We ignore if it fails because another thread will simply
                        // handle it in the future anyways
                    } else {
                        if (head.get().compare_exchange_weak(
                                first,
                                firstValuable,
                                std::memory_order_release,
                                std::memory_order_acquire
                            )) {
                            T value = firstValuable->get();
                            // We successfully retrieved the first element and replaced the head
                            // reference. Now cleaning the references.
                            epoch_reclamation.retire_reference(adopt(first));
                            return value;
                        }
                        // We lost the race condition to another thread - it dequeued before
                        // we did. Simply continue the loop to try one more time.
                        continue;
                    }
                } else {
                    // Consistency check failed, data is invalid - continue the loop to try one
                    // more time.
                    continue;
                }
            }
        }

    public:
        LockFreeUnboundedQueueImpl() {
            gsl::owner<LockFreeNode<T>*> dummy = create_dummy();
            head.get().store(dummy, std::memory_order_relaxed);
            tail.get().store(dummy, std::memory_order_relaxed);
        }

        ~LockFreeUnboundedQueueImpl() {
            // No other thread can reach the queue any more, so walk the chain directly rather than
            // through the dequeue loop. Each node owns its value.
            LockFreeNode<T>* node = head.get().load(std::memory_order_relaxed);
            while (node != nullptr) {
                gsl::owner<const LockFreeNode<T>*> doomed = adopt(node);
                node = node->next();
                delete doomed;
            }
        }

        LockFreeUnboundedQueueImpl(const LockFreeUnboundedQueueImpl&) = delete;
        LockFreeUnboundedQueueImpl& operator=(const LockFreeUnboundedQueueImpl&) = delete;
        LockFreeUnboundedQueueImpl(LockFreeUnboundedQueueImpl&&) = delete;
        LockFreeUnboundedQueueImpl& operator=(LockFreeUnboundedQueueImpl&&) = delete;

        void enqueue(std::unique_ptr<LockFreeNode<T>> node) {
            enqueue_node(std::move(node));
            items_signal.notify();
        }

        std::optional<T> try_dequeue() {
            return dequeue_node();
        }

        std::optional<T> wait_dequeue(const std::chrono::steady_clock::duration& timeout) {
            return items_signal.wait_for(timeout, [this]() {
                return try_dequeue();
            });
        }

        [[nodiscard]] bool is_empty() const {
            while (true) {
                const LockFreeNode<T>* first = head.get().load(std::memory_order_acquire);
                const LockFreeNode<T>* last = tail.get().load(std::memory_order_acquire);

                // Memory consistency check. We want to make sure that the first didn't change
                // while we were reading last.
                if (first == head.get().load(std::memory_order_acquire)) {
                    return first == last;
                }
            }
        }
    };

    template <typename T>
    class LockFreeUnboundedQueue final : public UnboundedQueue<T> {
    private:
        LockFreeUnboundedQueueImpl<T> impl;

    public:
        LockFreeUnboundedQueue()
            : UnboundedQueue<T>() {
        }

        ~LockFreeUnboundedQueue() override = default;

        LockFreeUnboundedQueue(LockFreeUnboundedQueue&& other) = delete;
        LockFreeUnboundedQueue& operator=(LockFreeUnboundedQueue&& other) = delete;
        LockFreeUnboundedQueue(const LockFreeUnboundedQueue& other) = delete;
        LockFreeUnboundedQueue& operator=(const LockFreeUnboundedQueue& other) = delete;

        void enqueue(T&& value) override {
            impl.enqueue(std::make_unique<LockFreeNode<T>>(std::move(value)));
        }

        std::optional<T> try_dequeue() override {
            return impl.try_dequeue();
        }

        std::optional<T> wait_dequeue(const std::chrono::steady_clock::duration& timeout) override {
            return impl.wait_dequeue(timeout);
        }

        std::future<std::optional<T>> wait_dequeue_async(
            const std::chrono::steady_clock::duration& timeout
        ) override {
            return std::async(std::launch::async, [this, timeout]() -> std::optional<T> {
                return this->impl.wait_dequeue(timeout);
            });
        }

        [[nodiscard]] bool is_empty() const override {
            return impl.is_empty();
        }
    };
}  // namespace multithreading::structures::unbounded_queue