#pragma once

#include <future>
#include <optional>

#include "./BoundedQueue.h"

namespace multithreading::structures::bounded_queue {

    template <typename T>
    class LockFreeBoundedQueue final : public BoundedQueue<T> {
    private:
        const size_t size_limit;
    public:
        explicit LockFreeBoundedQueue(size_t sizeLimit)
            : size_limit(sizeLimit)
        {}

        LockFreeBoundedQueue(LockFreeBoundedQueue&&) = delete;
        LockFreeBoundedQueue(const LockFreeBoundedQueue&) = delete;
        LockFreeBoundedQueue& operator=(const LockFreeBoundedQueue&) = delete;
        LockFreeBoundedQueue& operator=(LockFreeBoundedQueue&&) = delete;

        ~LockFreeBoundedQueue() override = default;

        std::optional<T> try_dequeue() override {

        }

        std::optional<T> wait_dequeue(
            const std::chrono::steady_clock::duration& timeout
        ) override {

        }

        std::future<std::optional<T>> wait_dequeue_async(
            const std::chrono::steady_clock::duration& timeout
        ) override {

        }

        bool try_enqueue(const T& value) override {

        }

        bool try_enqueue(T&& value) override {

        }


        bool is_empty(bool isPrecise) const override {

        }

        bool is_full(bool isPrecise) const override {

        }
    };
} // namespace multithreading::structures::bounded_queue