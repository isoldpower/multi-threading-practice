#include "../../include/threads/ThreadBarrier.h"

namespace multithreading::utilities::threads {

    ThreadBarrier::ThreadBarrier()
        : barrier(start_promise.get_future().share()) {
    }

    ThreadBarrier::~ThreadBarrier() {
        this->terminate();
    }

    void ThreadBarrier::wait() {
        this->barrier.wait();
    }

    void ThreadBarrier::kickstart() {
        std::call_once(release_once, [this]() {
            start_promise.set_value();
        });
    }

    void ThreadBarrier::terminate() {
        is_terminated.store(true, std::memory_order_release);

        std::call_once(release_once, [this]() {
            start_promise.set_value();
        });

        this->join();
    }

    void ThreadBarrier::join() {
        for (std::thread& thread : threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        threads.clear();
    }
}  // namespace multithreading::utilities::threads