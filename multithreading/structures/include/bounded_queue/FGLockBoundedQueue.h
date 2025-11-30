#pragma once

#include <future>
#include <optional>
#include <semaphore>

#include "./BoundedQueue.h"

namespace multithreading::structures::bounded_queue {

    template <typename T>
    struct FGNode {
    private:
        T value;
        FGNode<T>* next_node;
    public:
        explicit FGNode(const T& value)
            : value(value)
            , next_node(nullptr)
        {}

        explicit FGNode(T&& value)
            : value(std::move(value))
            , next_node(nullptr)
        {}

        FGNode()
            : value(T{})
            , next_node(nullptr)
        {}

        FGNode(const T& value, FGNode<T>* nextNode)
            : value(value)
            , next_node(nextNode)
        {}

        void setNext(FGNode<T>* node) {
            this->next_node = node;
        }

        [[nodiscard]] FGNode<T>* next() const {
            return next_node;
        }

        [[nodiscard]] const T& get() const {
            return value;
        }

        [[nodiscard]] T& get() {
            return value;
        }
    };

    template <typename T>
    class FGLockBoundedQueueImpl {
    private:
        const size_t size_limit;
        FGNode<T>* head;
        FGNode<T>* tail;

        std::atomic<size_t> size_counter;
        std::atomic<bool> is_shutdown;
        std::condition_variable enqueue_condition;
        mutable std::mutex tail_mu;
        mutable std::mutex head_mu;

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

        bool is_operations_allowed() const {
            return !is_shutdown.load(std::memory_order_acquire);
        }

        bool unsafe_is_empty() const {
            return head->next() == nullptr;
        }
    public:
        explicit FGLockBoundedQueueImpl(const size_t sizeLimit)
            : size_limit(sizeLimit)
        {
            head = tail = new FGNode<T>();
        }

        ~FGLockBoundedQueueImpl() {
            is_shutdown.store(true, std::memory_order_release);
            // Notify everyone that the state changed. They will wake up because now
            // operations are not allowed.
            enqueue_condition.notify_all();

            std::lock_guard head_lock(head_mu);
            std::lock_guard tail_lock(tail_mu);
            while (head != tail) {
                FGNode<T>* dummy = head;
                head = head->next();
                delete dummy;
            }
            delete tail;
        }

        std::optional<T> try_dequeue() {
            // Quick check to decrease amount of mutex-waiting when queue is obviously empty
            if (size_counter.load(std::memory_order_relaxed) == 0) {
                return std::nullopt;
            }

            std::lock_guard lock(head_mu);
            if (!is_operations_allowed() || head->next() == nullptr) {
                // Queue is empty or shutting down, even though our first counter check succeeded.
                // Probably means that the amount of elements were 1, and it decreased while we were
                // waiting for mutex unlock (was busy with dequeuing the last element).
                return std::nullopt;
            }

            auto result = unsafe_dequeue();
            size_counter.fetch_sub(1, std::memory_order_relaxed);
            return result;
        }

        std::optional<T> wait_dequeue(
            const std::chrono::steady_clock::duration& timeout
        ) {
            // Lock the mutex, wait for new item to be enqueued. Return std::nullopt if
            // the time limit exceeded.
            std::unique_lock lock(head_mu);
            if (!enqueue_condition.wait_for(lock, timeout, [this]() {
                return !is_operations_allowed() || !unsafe_is_empty();
            })) {
                return std::nullopt;
            }

            // Check if we woke up because operations are not allowed.
            if (!is_operations_allowed()) {
                return std::nullopt;
            }

            auto result = unsafe_dequeue();
            if (result.has_value()) {
                size_counter.fetch_sub(1, std::memory_order_relaxed);
            }

            return result;
        }

        bool try_enqueue(FGNode<T>* node) {
            // Quick check to decrease amount of mutex-waiting when queue is obviously full
            if (size_counter.load(std::memory_order_relaxed) >= size_limit) {
                return false;
            }

            std::lock_guard lock(tail_mu);
            if (!is_operations_allowed()) {
                return false;
            }
            // Acquire lock, ensure queue has space to add element and increment the counter
            // if it does. Rollback and return false otherwise.
            if (size_counter.fetch_add(1, std::memory_order_relaxed) >= size_limit) {
                size_counter.fetch_sub(1, std::memory_order_relaxed);
                return false;
            }

            tail->setNext(node);
            tail = node;

            enqueue_condition.notify_one();
            return true;
        }

        bool is_empty() const {
            return size_counter.load(std::memory_order_relaxed) == 0;
        }

        bool is_empty_precise() const {
            std::lock_guard lock(head_mu);
            return unsafe_is_empty();
        }

        bool is_full() const {
            return size_counter.load(std::memory_order_relaxed) >= size_limit;
        }

        bool is_full_precise() const {
            std::lock_guard lock(tail_mu);
            return is_full();
        }
    };

    template <typename T>
    class FGLockBoundedQueue final : public BoundedQueue<T> {
    private:
        FGLockBoundedQueueImpl<T> impl;
    public:
        explicit FGLockBoundedQueue(const size_t sizeLimit) noexcept
            : impl(sizeLimit)
        {}

        FGLockBoundedQueue(FGLockBoundedQueue&& other) = delete;
        FGLockBoundedQueue(const FGLockBoundedQueue& other) = delete;
        FGLockBoundedQueue& operator=(const FGLockBoundedQueue& other) = default;
        FGLockBoundedQueue& operator=(FGLockBoundedQueue&& other) = delete;

        ~FGLockBoundedQueue() override = default;

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
            return std::async(std::launch::async, [this, timeout]() {
                return this->impl.wait_dequeue(timeout);
            });
        }

        bool try_enqueue(const T& value) override {
            auto* node = new FGNode<T>(value);
            if (!impl.try_enqueue(node)) {
                delete node;
                return false;
            }

            return true;
        }

        bool try_enqueue(T&& value) override {
            auto* node = new FGNode<T>(std::move(value));
            if (!impl.try_enqueue(node)) {
                delete node;
                return false;
            }

            return true;
        }

        [[nodiscard]] bool is_empty(bool isPrecise) const override {
            return isPrecise ? impl.is_empty_precise() : impl.is_empty();
        }

        [[nodiscard]] bool is_full(bool isPrecise) const override {
            return isPrecise ? impl.is_full_precise() : impl.is_full();
        }
    };
} // namespace multithreading::structures::bounded_queue