#pragma once

#include <cstddef>

#include "./UnboundedQueue.h"

namespace multithreading::structures::unbounded_queue {

    template <typename T>
    struct FGNode {
    private:
        T value;
        FGNode* nextNode;
    public:
        explicit FGNode(const T& value)
            : value(value)
            , nextNode(nullptr)
        {}

        explicit FGNode(T&& value)
            : value(std::move(value))
            , nextNode(nullptr)
        {}

        FGNode()
            : value(T{})
            , nextNode(nullptr)
        {}

        [[nodiscard]] T& get() {
            return value;
        }

        [[nodiscard]] FGNode* next() const {
            return nextNode;
        }

        void setNext(FGNode* next) {
            this->nextNode = next;
        }
    };

    template <typename T>
    class FGLockUnboundedQueueImpl {
    private:
        mutable std::mutex head_mu;
        mutable std::mutex tail_mu;

        std::condition_variable enqueue_condition;

        FGNode<T>* head;
        FGNode<T>* tail;

        void unsafe_enqueue(FGNode<T>* node) {
            tail->setNext(node);
            tail = node;

            enqueue_condition.notify_one();
        }

        std::optional<T> unsafe_dequeue() {
            FGNode<T>* dummyNode = head;
            FGNode<T>* firstValuableNode = dummyNode->next();

            if (firstValuableNode == nullptr) {
                return std::nullopt;
            }

            std::optional<T> result = std::move(firstValuableNode->get());
            head = firstValuableNode;
            delete dummyNode;

            return result;
        }

        bool unsafe_is_empty() const {
            return head->next() == nullptr;
        }
    public:
        FGLockUnboundedQueueImpl() {
            head = tail = new FGNode<T>();
        }

        ~FGLockUnboundedQueueImpl() {
            std::lock_guard head_lock(head_mu);
            std::lock_guard tail_lock(tail_mu);

            while (true) {
                // Optimisation purpose. If we put the ID-dependant condition in the while loop,
                // it will decrease the performance.
                if (head == tail) {
                    break;
                }

                const FGNode<T>* dummy = head;
                head = head->next();
                delete dummy;
            }
            delete tail;
        }

        FGLockUnboundedQueueImpl(const FGLockUnboundedQueueImpl& other) = delete;
        FGLockUnboundedQueueImpl& operator=(const FGLockUnboundedQueueImpl& other) = delete;
        FGLockUnboundedQueueImpl(FGLockUnboundedQueueImpl&& other) = delete;
        FGLockUnboundedQueueImpl& operator=(FGLockUnboundedQueueImpl&& other) = delete;

        void enqueue(FGNode<T>* node) {
            std::lock_guard lock(tail_mu);

            unsafe_enqueue(node);
        }

        std::optional<T> try_dequeue() {
            std::lock_guard lock(head_mu);

            return unsafe_dequeue();
        }

        std::optional<T> wait_dequeue(
            const std::chrono::steady_clock::duration& timeout
        ) {
            std::unique_lock lock(head_mu);

            if (!enqueue_condition.wait_for(lock, timeout, [this]() {
                return !this->unsafe_is_empty();
            })) {
                return std::nullopt;
            }

            return unsafe_dequeue();
        }

        [[nodiscard]] bool is_empty() const {
            std::lock_guard lock(head_mu);
            return unsafe_is_empty();
        }
    };

    template <typename T>
    class FGLockUnboundedQueue final : public UnboundedQueue<T> {
    private:
        FGLockUnboundedQueueImpl<T> impl;
    public:
        FGLockUnboundedQueue() noexcept
            : UnboundedQueue<T>()
            , impl()
        {}

        FGLockUnboundedQueue(FGLockUnboundedQueue&& other) = delete;
        FGLockUnboundedQueue(const FGLockUnboundedQueue& other) = delete;
        FGLockUnboundedQueue& operator=(const FGLockUnboundedQueue& other) = delete;
        FGLockUnboundedQueue& operator=(FGLockUnboundedQueue&& other) = delete;

        ~FGLockUnboundedQueue() override = default;

        void enqueue(const T& value) override {
            auto* node = new FGNode<T>(value);
            impl.enqueue(node);
        }

        void enqueue(T&& value) override {
            auto* node = new FGNode<T>(std::move(value));
            impl.enqueue(node);
        }

        std::optional<T> try_dequeue() override {
            return impl.try_dequeue();
        }

        std::optional<T> wait_dequeue(
            const std::chrono::steady_clock::duration& timeout
        ) override {
            return impl.wait_dequeue(timeout);
        }

        std::future<std::optional<T>> wait_dequeue_async(
            const std::chrono::steady_clock::duration& timeout
        ) override {
            return std::async(std::launch::async, [this, timeout]() -> std::optional<T> {
                return impl.wait_dequeue(timeout);
            });
        }

        [[nodiscard]] bool is_empty() const override {
            return impl.is_empty();
        }
    };
} // namespace multithreading::structures::unbounded_queue