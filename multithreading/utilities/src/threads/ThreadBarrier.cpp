#include "../../include/threads/ThreadBarrier.h"


namespace multithreading::utilities::threads {

    ThreadBarrier::ThreadBarrier()
        : barrier(start_promise.get_future().share())
        , is_terminated(false)
    {}

    ThreadBarrier::~ThreadBarrier() {
        this->terminate();
    }

    void ThreadBarrier::wait() {
        this->barrier.wait();
    }

    void ThreadBarrier::kickstart() {
        start_promise.set_value();
    }

    void ThreadBarrier::terminate() {
        is_terminated.store(true, std::memory_order_release);

        try {
            start_promise.set_value();
        } catch (std::future_error& error) {
            // Can't terminate on a flight. Ignore the error
        }

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
} // namespace multithreading::utilities::threads