#include <iostream>
#include <ostream>

#include <multithreading/structures/include/unbounded_queue/FGLockUnboundedQueue.h>
#include <multithreading/structures/include/unbounded_queue/LockFreeUnboundedQueue.h>
#include <multithreading/structures/include/unbounded_queue/UnboundedQueue.h>
#include <multithreading/utilities/include/Application.h>

template <typename T>
static int runQueueMultithreading(
    multithreading::structures::unbounded_queue::UnboundedQueue<T>* queue
) {
    constexpr size_t PER_THREAD_SIZE = 1000;
    constexpr size_t DEQUEUE_TIMEOUT_MS = 1000;

    const size_t threadsCount = std::thread::hardware_concurrency();
    std::vector<std::thread> threads {};
    threads.reserve(threadsCount);

    for (size_t i = 0; i < threadsCount; ++i) {
        threads.emplace_back([&queue] {
            for (size_t j = 0; j < PER_THREAD_SIZE; j++) {
                queue->enqueue(static_cast<T>(j));
            }
        });
    }

    for (size_t i = 0; i < PER_THREAD_SIZE * threadsCount; ++i) {
        std::future<std::optional<T>> latestValue = queue->wait_dequeue_async(
            std::chrono::milliseconds(DEQUEUE_TIMEOUT_MS)
        );

        const std::optional<T> value = latestValue.get();
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

    const std::array<
        std::unique_ptr<multithreading::structures::unbounded_queue::UnboundedQueue<int>>, 2
    > queues {
        std::make_unique<multithreading::structures::unbounded_queue::FGLockUnboundedQueue<int>>(),
        std::make_unique<multithreading::structures::unbounded_queue::LockFreeUnboundedQueue<int>>(
            multithreading::structures::unbounded_queue::LockFreeQueueConfig{
                .maxUpdateDepth = 10000
            }
        )
    };

    const std::optional<int> executionResult = benchmarkApplication.SafeStart(
        [&]() -> int {
            return runQueueMultithreading<int>(queues.at(0).get());
        }
    );
    const std::optional<int> executionResult2 = benchmarkApplication.SafeStart(
        [&]() -> int {
            return runQueueMultithreading<int>(queues.at(1).get());
        }
    );

    try {
        if (executionResult.has_value()) {
            return executionResult.value();
        }
        if (executionResult2.has_value()) {
            return executionResult2.value();
        }

        return -1;
    } catch (std::exception& e) {
        std::cerr << e.what() << '\n';

        return -1;
    }
}