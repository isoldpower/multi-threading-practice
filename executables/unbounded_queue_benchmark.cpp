#include <multithreading/structures/include/unbounded_queue/FGLockUnboundedQueue.h>

#include <iostream>
#include <ostream>

#include "multithreading/utilities/include/Application.h"


static int runQueueMultithreading() {
    constexpr size_t PER_THREAD_SIZE = 1000;
    constexpr size_t DEQUEUE_TIMEOUT_MS = 1000;

    multithreading::structures::unbounded_queue::FGLockUnboundedQueue<int> queue;

    const size_t threadsCount = std::thread::hardware_concurrency();
    std::vector<std::thread> threads {};
    threads.reserve(threadsCount);

    for (size_t i = 0; i < threadsCount; ++i) {
        threads.emplace_back([&queue] {
            for (size_t j = 0; j < PER_THREAD_SIZE; j++) {
                queue.enqueue(static_cast<int>(j));
            }
        });
    }

    for (size_t i = 0; i < PER_THREAD_SIZE * threadsCount; ++i) {
        std::future<std::optional<int>> latestValue = queue.wait_dequeue(
            std::chrono::milliseconds(DEQUEUE_TIMEOUT_MS)
        );

        const std::optional<int> value = latestValue.get();
        if (value.has_value()) {
            std::cout << "Dequeued at iteration " << i + 1 << "(" << value.value() << ")" << '\n';
        } else {
            i--;
        }
    }

    for (size_t i = 0; i < threadsCount; ++i) {
        if (threads[i].joinable()) {
            threads[i].join();
        }
    }

    return 0;
}

auto main() -> int {
    multithreading::utilities::Application<int> benchmarkApplication(
        multithreading::utilities::ApplicationInfo{
            .appName="Unbounded Queue Benchmark",
            .appVersion="1.0.0"
        }
    );

    const std::optional<int> executionResult = benchmarkApplication.SafeStart(
        runQueueMultithreading
    );

    try {
        return executionResult.has_value() ? executionResult.value() : -1;
    } catch (std::exception& e) {
        std::cerr << e.what() << '\n';
        return -1;
    }
}