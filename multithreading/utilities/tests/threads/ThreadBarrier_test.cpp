#include <gtest/gtest.h>

#include "../../include/threads/ThreadBarrier.h"

using multithreading::utilities::threads::ThreadBarrier;

TEST(ThreadBarrierTest, ThreadsBlockUntilKickstart) {
    ThreadBarrier barrier;
    std::atomic<bool> executed{false};

    barrier.enqueue([&executed]() {
        executed.store(true, std::memory_order_release);
    });

    std::this_thread::yield();
    EXPECT_FALSE(executed.load(std::memory_order_acquire));

    barrier.kickstart();
    barrier.join();
    EXPECT_TRUE(executed.load(std::memory_order_acquire));
}

TEST(ThreadBarrierTest, AllThreadsExecuteAfterKickstart) {
    constexpr size_t THREAD_COUNT = 8;
    ThreadBarrier barrier;
    std::atomic<size_t> executed{0};

    for (size_t i = 0; i < THREAD_COUNT; ++i) {
        barrier.enqueue([&executed]() {
            executed.fetch_add(1, std::memory_order_release);
        });
    }

    barrier.kickstart();
    barrier.join();

    EXPECT_EQ(executed.load(std::memory_order_acquire), THREAD_COUNT);
}

TEST(ThreadBarrierTest, EnqueueReturnsValidDistinctPointers) {
    ThreadBarrier barrier;

    const std::thread* thread_first = barrier.enqueue([](){});
    const std::thread* thread_second = barrier.enqueue([](){});

    EXPECT_NE(thread_first, nullptr);
    EXPECT_NE(thread_second, nullptr);
    EXPECT_NE(thread_first, thread_second);

    barrier.kickstart();
    barrier.join();
}

TEST(ThreadBarrierTest, TerminatePreventsTaksExecution) {
    constexpr size_t THREAD_COUNT = 4;
    ThreadBarrier barrier;
    std::atomic<size_t> executed{0};

    for (size_t i = 0; i < THREAD_COUNT; ++i) {
        barrier.enqueue([&executed]() {
            executed.fetch_add(1, std::memory_order_release);
        });
    }

    barrier.terminate();
    barrier.join();

    EXPECT_EQ(executed.load(std::memory_order_acquire), 0);
}

TEST(ThreadBarrierTest, WaitBlocksUntilKickstart) {
    ThreadBarrier barrier;
    std::atomic<bool> wait_returned{false};

    std::thread waiter([&barrier, &wait_returned]() {
        barrier.wait();
        wait_returned.store(true, std::memory_order_release);
    });

    std::this_thread::yield();
    EXPECT_FALSE(wait_returned.load(std::memory_order_acquire));

    barrier.kickstart();
    waiter.join();
    EXPECT_TRUE(wait_returned.load(std::memory_order_acquire));
}

TEST(ThreadBarrierTest, TasksExecuteConcurrentlyAfterKickstart) {
    constexpr size_t THREAD_COUNT = 8;
    ThreadBarrier barrier;
    std::atomic<size_t> peak{0};
    std::atomic<size_t> active{0};

    std::mutex sync_mutex;
    std::condition_variable sync_cv;
    std::atomic<size_t> arrived{0};

    for (size_t i = 0; i < THREAD_COUNT; ++i) {
        barrier.enqueue([&]() {
            const size_t current = active.fetch_add(1, std::memory_order_release) + 1;
            size_t prev = peak.load(std::memory_order_acquire);
            while (current > prev && !peak.compare_exchange_weak(
                prev, current,
                std::memory_order_release,
                std::memory_order_acquire
            )) {}

            {
                std::unique_lock lock(sync_mutex);
                arrived.fetch_add(1, std::memory_order_release);
                sync_cv.wait(lock, [&]() {
                    return arrived.load(std::memory_order_acquire) == THREAD_COUNT;
                });
            }
            sync_cv.notify_all();

            active.fetch_sub(1, std::memory_order_release);
        });
    }

    barrier.kickstart();
    barrier.join();

    EXPECT_EQ(peak.load(std::memory_order_acquire), THREAD_COUNT);
}

TEST(ThreadBarrierTest, JoinBlocksUntilAllThreadsComplete) {
    constexpr size_t THREAD_COUNT = 4;
    ThreadBarrier barrier;
    std::atomic<size_t> completed{0};

    for (size_t i = 0; i < THREAD_COUNT; ++i) {
        barrier.enqueue([&completed]() {
            std::this_thread::yield();
            completed.fetch_add(1, std::memory_order_release);
        });
    }

    barrier.kickstart();
    barrier.join();

    EXPECT_EQ(completed.load(std::memory_order_acquire), THREAD_COUNT);
}

TEST(ThreadBarrierTest, EmptyTaskDoesNotCrash) {
    ThreadBarrier barrier;

    for (size_t i = 0; i < 4; ++i) {
        barrier.enqueue([](){});
    }

    barrier.kickstart();
    barrier.join();
}