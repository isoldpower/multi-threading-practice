#include <iostream>
#include <ostream>

#include <multithreading/structures/include/bounded_queue/FGLockBoundedQueue.h>
#include <multithreading/structures/include/bounded_queue/LockFreeBoundedQueue.h>
#include <multithreading/structures/include/bounded_queue/BoundedQueue.h>
#include <multithreading/utilities/include/Application.h>

constexpr size_t PER_THREAD_SIZE = 1000;
constexpr size_t DEQUEUE_TIMEOUT_MS = 1000;
constexpr size_t QUEUE_SIZE = 10000;

template <typename T>
static int runQueueMultithreading(
    multithreading::structures::bounded_queue::BoundedQueue<T>* queue
) {
    const size_t threadsCount = std::thread::hardware_concurrency();
    std::vector<std::thread> threads {};
    threads.reserve(threadsCount);

    for (size_t i = 0; i < threadsCount; ++i) {
        threads.emplace_back([&queue] {
            for (size_t j = 0; j < PER_THREAD_SIZE; j++) {
                queue->try_enqueue(static_cast<T>(j));
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
            .appName="Bounded Queue Benchmark",
            .appVersion="1.0.0"
        }
    );

    const std::array<
        std::unique_ptr<multithreading::structures::bounded_queue::BoundedQueue<int>>, 1
    > queues {
        std::make_unique<multithreading::structures::bounded_queue::FGLockBoundedQueue<int>>(QUEUE_SIZE)
    };

    const std::optional<int> executionResult = benchmarkApplication.SafeStart(
        [&]() -> int {
            return runQueueMultithreading<int>(queues.at(0).get());
        }
    );

    try {
        if (executionResult.has_value()) {
            return executionResult.value();
        }

        return -1;
    } catch (std::exception& e) {
        std::cerr << e.what() << '\n';

        return -1;
    }
}