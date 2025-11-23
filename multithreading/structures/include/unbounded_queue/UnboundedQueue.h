#pragma once

#include <future>
#include <chrono>


namespace multithreading::structures::unbounded_queue {

    template <typename T>
    class UnboundedQueue {
    public:
        virtual std::optional<T> try_dequeue() = 0;
        virtual std::future<std::optional<T>> wait_dequeue(const std::chrono::milliseconds& timeout) = 0;
        virtual void enqueue(const T& value) = 0;
        virtual void enqueue(T&& value) = 0;

        [[nodiscard]] virtual bool is_empty() const = 0;
    };
} // namespace multithreading::structures::unbounded_queue