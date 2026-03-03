#include <gtest/gtest.h>

#include "../../include/threads/CountThreadBarrier.h"

using multithreading::utilities::threads::CountThreadBarrier;

TEST(CountThreadBarrierTest, KickstartsAfterTargetCountReached) {
    constexpr size_t THREAD_COUNT = 4;
    CountThreadBarrier barrier(THREAD_COUNT);
    std::atomic<size_t> started{0};

    for (size_t i = 0; i < THREAD_COUNT; ++i) {
        barrier.enqueue([&started]() {
            started.fetch_add(1, std::memory_order_release);
        });
    }

    barrier.join();
    EXPECT_EQ(started.load(std::memory_order_acquire), THREAD_COUNT);
}

TEST(CountThreadBarrierTest, ThreadsBlockUntilTargetCountReached) {
    constexpr size_t THREAD_COUNT = 4;
    CountThreadBarrier barrier(THREAD_COUNT);
    std::atomic<size_t> started{0};

    for (size_t i = 0; i < THREAD_COUNT - 1; ++i) {
        barrier.enqueue([&started]() {
            started.fetch_add(1, std::memory_order_release);
        });
    }

    std::this_thread::yield();
    EXPECT_EQ(started.load(std::memory_order_acquire), 0);

    barrier.enqueue([&started]() {
        started.fetch_add(1, std::memory_order_release);
    });
    barrier.join();

    EXPECT_EQ(started.load(std::memory_order_acquire), THREAD_COUNT);
}

TEST(CountThreadBarrierTest, EnqueueReturnsValidThreadPointer) {
    constexpr size_t THREAD_COUNT = 2;
    CountThreadBarrier barrier(THREAD_COUNT);

    const std::thread* thread_first = barrier.enqueue([](){});
    const std::thread* thread_second = barrier.enqueue([](){});

    EXPECT_NE(thread_first, nullptr);
    EXPECT_NE(thread_second, nullptr);
    EXPECT_NE(thread_first, thread_second);

    barrier.join();
}

TEST(CountThreadBarrierTest, EnqueueAndWaitBlocksUntilCompletion) {
    constexpr size_t THREAD_COUNT = 4;
    CountThreadBarrier barrier(THREAD_COUNT);
    std::atomic<size_t> completed{0};

    for (size_t i = 0; i < THREAD_COUNT - 1; ++i) {
        barrier.enqueue([&completed]() {
            std::this_thread::yield();
            completed.fetch_add(1, std::memory_order_release);
        });
    }

    barrier.enqueue_and_wait([&completed]() {
        std::this_thread::yield();
        completed.fetch_add(1, std::memory_order_release);
    });
    barrier.join();

    EXPECT_EQ(completed.load(std::memory_order_acquire), THREAD_COUNT);
}

TEST(CountThreadBarrierTest, EmptyEnqueueDoesNotCrash) {
    constexpr size_t THREAD_COUNT = 4;
    CountThreadBarrier barrier(THREAD_COUNT);

    for (size_t i = 0; i < THREAD_COUNT; ++i) {
        barrier.enqueue();
    }
    barrier.join();
}

TEST(CountThreadBarrierTest, EmptyEnqueueAndWaitDoesNotCrash) {
    constexpr size_t THREAD_COUNT = 4;
    CountThreadBarrier barrier(THREAD_COUNT);

    for (size_t i = 0; i < THREAD_COUNT - 1; ++i) {
        barrier.enqueue();
    }
    barrier.enqueue_and_wait();
}

TEST(CountThreadBarrierTest, ConcurrentEnqueuesAreSafe) {
    constexpr size_t OUTER_THREADS = 4;
    constexpr size_t INNER_THREADS = OUTER_THREADS * 2;
    CountThreadBarrier barrier(INNER_THREADS);
    std::atomic<size_t> executed{0};

    std::vector<std::thread> enqueuers;
    enqueuers.reserve(OUTER_THREADS);
    for (size_t i = 0; i < OUTER_THREADS; ++i) {
        enqueuers.emplace_back([&barrier, &executed]() {
            barrier.enqueue([&executed]() {
                executed.fetch_add(1, std::memory_order_release);
            });
            barrier.enqueue([&executed]() {
                executed.fetch_add(1, std::memory_order_release);
            });
        });
    }

    for (auto& t : enqueuers) {
        t.join();
    }
    barrier.join();

    EXPECT_EQ(executed.load(std::memory_order_acquire), INNER_THREADS);
}

TEST(CountThreadBarrierTest, AllThreadsExecuteExactlyOnce) {
    constexpr size_t THREAD_COUNT = 8;
    CountThreadBarrier barrier(THREAD_COUNT);
    std::atomic<size_t> execution_count{0};

    for (size_t i = 0; i < THREAD_COUNT; ++i) {
        barrier.enqueue([&execution_count]() {
            execution_count.fetch_add(1, std::memory_order_release);
        });
    }

    barrier.join();

    EXPECT_EQ(execution_count.load(std::memory_order_acquire), THREAD_COUNT);
}