#pragma once

#include <future>
#include <optional>


namespace multithreading::structures::bounded_queue {

    template <typename T>
    class BoundedQueue {
    public:
        virtual ~BoundedQueue() = default;

        virtual std::optional<T> try_dequeue() = 0;
        virtual std::optional<T> wait_dequeue(
            const std::chrono::steady_clock::duration& timeout
        ) = 0;
        virtual std::future<std::optional<T>> wait_dequeue_async(
            const std::chrono::steady_clock::duration& timeout
        ) = 0;
        virtual bool try_enqueue(const T& value) = 0;
        virtual bool try_enqueue(T&& value) = 0;

        [[nodiscard]] virtual bool is_empty(bool isPrecise) const = 0;
        [[nodiscard]] virtual bool is_full(bool isPrecise) const = 0;
    };
} // namespace multithreading::structures::bounded_queue