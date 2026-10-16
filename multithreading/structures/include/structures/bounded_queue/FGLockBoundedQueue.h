#pragma once

#include <structures/bounded_queue/BoundedQueue.h>
#include <utilities/performance/Owner.h>

#include <atomic>
#include <condition_variable>
#include <future>
#include <mutex>
#include <optional>

namespace multithreading::structures::bounded_queue {

    constexpr size_t FG_NODE_ALIGNMENT = 16;

    template <typename T>
    struct alignas(FG_NODE_ALIGNMENT) FGNode {
    private:
        T value{};
        FGNode<T>* next_node{nullptr};

    public:
        explicit FGNode(const T& value)
            : value(value) {
        }

        explicit FGNode(T&& value)
            : value(std::move(value)) {
        }

        FGNode() = default;

        FGNode(const T& value, FGNode<T>* nextNode)
            : value(value)
            , next_node(nextNode) {
        }

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
        gsl::owner<FGNode<T>*> head;
        FGNode<T>* tail;

        std::atomic<size_t> size_counter{0};
        std::atomic<bool> is_shutdown{false};
        std::condition_variable enqueue_condition;
        mutable std::mutex tail_mu;
        mutable std::mutex head_mu;

        static gsl::owner<FGNode<T>*> create_dummy() {
            return new FGNode<T>();
        }

        static gsl::owner<FGNode<T>*> create_node(T&& value) {
            return new FGNode<T>(std::move(value));
        }

        static gsl::owner<FGNode<T>*> adopt(FGNode<T>* node) noexcept {
            return node;
        }

        bool try_link(FGNode<T>* node) {
            // Quick check to decrease amount of mutex-waiting when queue is obviously full
            if (size_counter.load(std::memory_order_relaxed) >= size_limit) {
                return false;
            }

            const std::scoped_lock lock(tail_mu);
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

        bool is_operations_allowed() const {
            return !is_shutdown.load(std::memory_order_acquire);
        }

        bool unsafe_is_empty() const {
            return head->next() == nullptr;
        }

    public:
        explicit FGLockBoundedQueueImpl(const size_t sizeLimit)
            : size_limit(sizeLimit)
            , head(create_dummy())
            , tail(head) {
        }

        ~FGLockBoundedQueueImpl() {
            is_shutdown.store(true, std::memory_order_release);
            // Notify everyone that the state changed. They will wake up because now
            // operations are not allowed.
            enqueue_condition.notify_all();

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

        FGLockBoundedQueueImpl(const FGLockBoundedQueueImpl& other) = delete;
        FGLockBoundedQueueImpl& operator=(const FGLockBoundedQueueImpl& other) = delete;
        FGLockBoundedQueueImpl(FGLockBoundedQueueImpl&& other) = delete;
        FGLockBoundedQueueImpl& operator=(FGLockBoundedQueueImpl&& other) = delete;

        std::optional<T> try_dequeue() {
            // Quick check to decrease amount of mutex-waiting when queue is obviously empty
            if (size_counter.load(std::memory_order_relaxed) == 0) {
                return std::nullopt;
            }

            const std::scoped_lock lock(head_mu);
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

        std::optional<T> wait_dequeue(const std::chrono::steady_clock::duration& timeout) {
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

        bool try_enqueue(T&& value) {
            gsl::owner<FGNode<T>*> node = create_node(std::move(value));
            if (try_link(node)) {
                return true;
            }

            // Hand the value back so a failed enqueue leaves the caller's value untouched.
            value = std::move(node->get());
            delete node;
            return false;
        }

        bool is_empty() const {
            return size_counter.load(std::memory_order_relaxed) == 0;
        }

        bool is_empty_precise() const {
            const std::scoped_lock lock(head_mu);
            return unsafe_is_empty();
        }

        bool is_full() const {
            return size_counter.load(std::memory_order_relaxed) >= size_limit;
        }

        bool is_full_precise() const {
            const std::scoped_lock lock(tail_mu);
            return is_full();
        }
    };

    template <typename T>
    class FGLockBoundedQueue final : public BoundedQueue<T> {
    private:
        FGLockBoundedQueueImpl<T> impl;

    public:
        explicit FGLockBoundedQueue(const size_t sizeLimit) noexcept
            : BoundedQueue<T>()
            , impl(sizeLimit) {
        }

        FGLockBoundedQueue(FGLockBoundedQueue&& other) = delete;
        FGLockBoundedQueue(const FGLockBoundedQueue& other) = delete;
        FGLockBoundedQueue& operator=(const FGLockBoundedQueue& other) = default;
        FGLockBoundedQueue& operator=(FGLockBoundedQueue&& other) = delete;

        ~FGLockBoundedQueue() override = default;

        std::optional<T> try_dequeue() override {
            return impl.try_dequeue();
        }

        std::optional<T> wait_dequeue(const std::chrono::steady_clock::duration& timeout) override {
            return impl.wait_dequeue(timeout);
        }

        std::future<std::optional<T>> wait_dequeue_async(
            const std::chrono::steady_clock::duration& timeout
        ) override {
            return std::async(std::launch::async, [this, timeout]() {
                return this->impl.wait_dequeue(timeout);
            });
        }

        bool try_enqueue(T&& value) override {
            return impl.try_enqueue(std::move(value));
        }

        [[nodiscard]] bool is_empty(bool isPrecise) const override {
            return isPrecise ? impl.is_empty_precise() : impl.is_empty();
        }

        [[nodiscard]] bool is_full(bool isPrecise) const override {
            return isPrecise ? impl.is_full_precise() : impl.is_full();
        }
    };
}  // namespace multithreading::structures::bounded_queue