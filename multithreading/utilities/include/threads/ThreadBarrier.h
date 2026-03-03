#pragma once

#include <future>
#include <iostream>
#include <thread>

namespace multithreading::utilities::threads {

    class ThreadBarrier {
    private:
        std::promise<void> start_promise;
        std::shared_future<void> barrier;
        std::atomic<bool> is_terminated;

        std::vector<std::thread> threads;
    public:
        ThreadBarrier();
        ~ThreadBarrier();

        template <typename TCallable>
        std::thread* enqueue(TCallable&& task) {
            auto wait_thread = std::thread([this, task = std::forward<TCallable>(task)]() {
                barrier.wait();

                if (!is_terminated.load(std::memory_order_acquire)) {
                    task();
                }
            });
            threads.push_back(std::move(wait_thread));

            return &threads.back();
        }

        void wait();

        void kickstart();

        void terminate();

        void join();
    };
} // namespace multithreading::utilities::threads