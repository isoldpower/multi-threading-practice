#pragma once

#include <future>
#include <optional>

namespace multithreading::structures::unbounded_queue {

    template <typename T>
    class UnboundedQueue {
    public:
        virtual ~UnboundedQueue() = default;

        UnboundedQueue(const UnboundedQueue&) = delete;
        UnboundedQueue& operator=(const UnboundedQueue&) = delete;
        UnboundedQueue(UnboundedQueue&&) = delete;
        UnboundedQueue& operator=(UnboundedQueue&&) = delete;

        virtual std::optional<T> try_dequeue() = 0;
        virtual std::optional<T> wait_dequeue(
            const std::chrono::steady_clock::duration& timeout
        ) = 0;
        virtual std::future<std::optional<T>> wait_dequeue_async(
            const std::chrono::steady_clock::duration& timeout
        ) = 0;
        virtual void enqueue(const T& value) = 0;
        virtual void enqueue(T&& value) = 0;

        [[nodiscard]] virtual bool is_empty() const = 0;
    };
} // namespace multithreading::structures::unbounded_queue