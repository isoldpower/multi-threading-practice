#pragma once

#include <functional>
#include <future>
#include <iostream>
#include <thread>
#include <tuple>

namespace multithreading::utilities::threads {

    class ThreadBarrier {
    private:
        std::promise<void> start_promise;
        std::shared_future<void> barrier;
        std::atomic<bool> is_terminated;

        std::vector<std::thread> threads;
    public:
        ThreadBarrier()
            : barrier(start_promise.get_future().share())
            , is_terminated(false)
        {}

        ~ThreadBarrier() {
            this->terminate();
        }

        template <typename TCallable>
        std::thread* enqueue(TCallable&& task) {
            auto wait_thread = std::thread([this, task = std::forward<TCallable>(task)]() {
                // Wait or terminate if is_terminated set to true
                barrier.wait();

                if (!is_terminated.load(std::memory_order_acquire)) {
                    task();
                }
            });
            threads.push_back(std::move(wait_thread));

            return &threads.back();
        }

        void kickstart() {
            start_promise.set_value();
        }

        void terminate() {
            is_terminated.store(true, std::memory_order_release);

            try {
                start_promise.set_value();
            } catch (std::future_error& error) {
                // Can't terminate on a flight. Ignore the error
            }

            this->join();
        }

        void join() {
            for (std::thread& thread : threads) {
                if (thread.joinable()) {
                    thread.join();
                }
            }
            threads.clear();
        }
    };
} // namespace multithreading::utilities::threads