#pragma once

#include <multithreading/utilities/include/performance/Owner.h>

#include <condition_variable>
#include <cstdint>
#include <future>
#include <mutex>
#include <optional>

#include "./UnboundedQueue.h"

namespace multithreading::structures::unbounded_queue {

    constexpr size_t FG_NODE_ALIGNMENT = 16;

    template <typename T>
    struct alignas(FG_NODE_ALIGNMENT) FGNode {
    private:
        T value{};
        FGNode* nextNode{nullptr};

    public:
        explicit FGNode(const T& value)
            : value(value) {
        }

        explicit FGNode(T&& value)
            : value(std::move(value)) {
        }

        FGNode() = default;

        [[nodiscard]] T& get() {
            return value;
        }

        [[nodiscard]] FGNode* next() const {
            return nextNode;
        }

        void set_next(FGNode* next) {
            this->nextNode = next;
        }
    };

    template <typename T>
    class FGLockUnboundedQueueImpl {
    private:
        mutable std::mutex head_mu;
        mutable std::mutex tail_mu;

        std::condition_variable enqueue_condition;

        gsl::owner<FGNode<T>*> head;
        FGNode<T>* tail;

        static gsl::owner<FGNode<T>*> create_dummy() {
            return new FGNode<T>();
        }

        static gsl::owner<FGNode<T>*> create_node(T&& value) {
            return new FGNode<T>(std::move(value));
        }

        static gsl::owner<FGNode<T>*> adopt(FGNode<T>* node) noexcept {
            return node;
        }

        void unsafe_enqueue(FGNode<T>* node) {
            tail->set_next(node);
            tail = node;

            enqueue_condition.notify_one();
        }

        std::optional<T> unsafe_dequeue() {
            gsl::owner<const FGNode<T>*> dummyNode = head;
            FGNode<T>* firstValuableNode = dummyNode->next();

            if (firstValuableNode == nullptr) {
                return std::nullopt;
            }

            std::optional<T> result = std::move(firstValuableNode->get());
            head = adopt(firstValuableNode);
            delete dummyNode;

            return result;
        }

        bool unsafe_is_empty() const {
            return head->next() == nullptr;
        }

    public:
        FGLockUnboundedQueueImpl()
            : head(create_dummy())
            , tail(head) {
        }

        ~FGLockUnboundedQueueImpl() {
            const std::scoped_lock lock(head_mu, tail_mu);

            while (true) {
                // Optimisation purpose. If we put the ID-dependant condition in the while loop,
                // it will decrease the performance.
                if (head == tail) {
                    break;
                }

                gsl::owner<const FGNode<T>*> dummy = head;
                head = adopt(head->next());
                delete dummy;
            }
            // The walk stops on the tail, which head now owns.
            delete head;
        }

        FGLockUnboundedQueueImpl(const FGLockUnboundedQueueImpl& other) = delete;
        FGLockUnboundedQueueImpl& operator=(const FGLockUnboundedQueueImpl& other) = delete;
        FGLockUnboundedQueueImpl(FGLockUnboundedQueueImpl&& other) = delete;
        FGLockUnboundedQueueImpl& operator=(FGLockUnboundedQueueImpl&& other) = delete;

        void enqueue(T&& value) {
            // Allocate before taking the lock to keep the critical section short.
            gsl::owner<FGNode<T>*> node = create_node(std::move(value));
            const std::scoped_lock lock(tail_mu);

            unsafe_enqueue(node);
        }

        std::optional<T> try_dequeue() {
            const std::scoped_lock lock(head_mu);

            return unsafe_dequeue();
        }

        std::optional<T> wait_dequeue(const std::chrono::steady_clock::duration& timeout) {
            std::unique_lock lock(head_mu);

            if (!enqueue_condition.wait_for(lock, timeout, [this]() {
                    return !this->unsafe_is_empty();
                })) {
                return std::nullopt;
            }

            return unsafe_dequeue();
        }

        [[nodiscard]] bool is_empty() const {
            const std::scoped_lock lock(head_mu);
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
            , impl() {
        }

        FGLockUnboundedQueue(FGLockUnboundedQueue&& other) = delete;
        FGLockUnboundedQueue(const FGLockUnboundedQueue& other) = delete;
        FGLockUnboundedQueue& operator=(const FGLockUnboundedQueue& other) = delete;
        FGLockUnboundedQueue& operator=(FGLockUnboundedQueue&& other) = delete;

        ~FGLockUnboundedQueue() override = default;

        void enqueue(T&& value) override {
            impl.enqueue(std::move(value));
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
                return impl.wait_dequeue(timeout);
            });
        }

        [[nodiscard]] bool is_empty() const override {
            return impl.is_empty();
        }
    };
}  // namespace multithreading::structures::unbounded_queue