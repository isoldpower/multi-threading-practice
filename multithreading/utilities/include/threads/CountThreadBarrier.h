#pragma once

#include "./ThreadBarrier.h"
#include <atomic>
#include <cstdint>

namespace multithreading::utilities::threads {

    class CountThreadBarrier {
    private:
        const size_t target_count;
        std::atomic<size_t> enqueued_size;
        ThreadBarrier barrier;
        std::mutex enqueue_mutex;
    public:
        explicit CountThreadBarrier(const size_t count)
            : target_count(count)
            , enqueued_size(0)
        {}

        ~CountThreadBarrier() = default;

        template <typename TCallable>
        std::thread* enqueue(TCallable&& task) {
            std::thread* fresh_thread = nullptr;

            {
                const std::scoped_lock lock(this->enqueue_mutex);
                fresh_thread = barrier.enqueue(std::forward<TCallable>(task));
            }
            auto previous_count = this->enqueued_size.fetch_add(1, std::memory_order_release);

            if (previous_count == this->target_count - 1) {
                auto future_value = std::async(std::launch::async, [this]() {
                    this->barrier.kickstart();
                });
            }

            return fresh_thread;
        }

        std::thread* enqueue() {
            return this->enqueue([](){});
        }

        template <typename TCallable>
        std::thread* enqueue_and_wait(TCallable&& task) {
            auto* thread = this->enqueue(std::forward<TCallable>(task));

            barrier.wait();
            return thread;
        }

        std::thread* enqueue_and_wait() {
            auto* thread = this->enqueue();

            barrier.wait();
            return thread;
        }

        void join() {
            barrier.join();
        }
    };
} // namespace multithreading::utilities::threads