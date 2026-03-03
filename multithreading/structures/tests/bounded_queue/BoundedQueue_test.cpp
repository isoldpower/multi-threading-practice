#include <gtest/gtest.h>
#include <multithreading/utilities/include/tests/AmorphicTest.h>
#include <multithreading/utilities/include/threads/ThreadBarrier.h>

#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <thread>
#include <vector>

#include "../../include/bounded_queue/FGLockBoundedQueue.h"
#include "../../include/bounded_queue/LockFreeBoundedQueue.h"

constexpr size_t DEFAULT_QUEUE_SIZE = 10;


template <typename T>
class BoundedQueueTest : public multithreading::utilities::tests::AmorphicTest<T> {
protected:
    std::unique_ptr<T> queue;

    void SetUp() override {
        queue = std::make_unique<T>(DEFAULT_QUEUE_SIZE);
    }

    void TearDown() override {
        queue = nullptr;
    }
};

using BoundedQueueImplementations = ::testing::Types<
    multithreading::structures::bounded_queue::FGLockBoundedQueue<int>,
    multithreading::structures::bounded_queue::FGLockBoundedQueue<std::unique_ptr<int>>,
    multithreading::structures::bounded_queue::LockFreeBoundedQueue<int>,
    multithreading::structures::bounded_queue::LockFreeBoundedQueue<std::unique_ptr<int>>
>;
TYPED_TEST_SUITE(BoundedQueueTest, BoundedQueueImplementations);

TYPED_TEST(BoundedQueueTest, StartsEmpty) {
    EXPECT_TRUE(this->queue->is_empty(false));
}

TYPED_TEST(BoundedQueueTest, StartsEmptyPrecise) {
    EXPECT_TRUE(this->queue->is_empty(true));
}

TYPED_TEST(BoundedQueueTest, EmptyDequeueNullopt) {
    using ValueType = multithreading::utilities::tests::QueueValueType<TypeParam>::type;

    const std::optional<ValueType> dequeued_value = this->queue->try_dequeue();
    EXPECT_FALSE(dequeued_value.has_value());
}

TYPED_TEST(BoundedQueueTest, EnqueueElementSuccess) {
    auto value = this->make_value(1);

    EXPECT_TRUE(this->queue->try_enqueue(std::move(value)));
}

TYPED_TEST(BoundedQueueTest, SizeReflectsEnqueue) {
    auto value = this->make_value(1);

    EXPECT_TRUE(this->queue->try_enqueue(std::move(value)));
    EXPECT_FALSE(this->queue->is_empty(false));
}

TYPED_TEST(BoundedQueueTest, FilledQueueIsFull) {
    for (size_t i = 0; i < DEFAULT_QUEUE_SIZE; ++i) {
        auto value = this->make_value(i);

        EXPECT_TRUE(this->queue->try_enqueue(std::move(value)));
    }

    EXPECT_FALSE(this->queue->is_empty(false));
    EXPECT_TRUE(this->queue->is_full(false));
}

TYPED_TEST(BoundedQueueTest, DequeueNotEmpty) {
    using ValueType = multithreading::utilities::tests::QueueValueType<TypeParam>::type;

    auto value = this->make_value(1);
    EXPECT_TRUE(this->queue->try_enqueue(std::move(value)));

    const std::optional<ValueType> dequeued_value = this->queue->try_dequeue();
    EXPECT_TRUE(dequeued_value.has_value());
}

TYPED_TEST(BoundedQueueTest, FifoOperationsOrder) {
    using ValueType = multithreading::utilities::tests::QueueValueType<TypeParam>::type;
    constexpr size_t TEST_SIZE = 2;

    for (size_t i = 0; i < TEST_SIZE; ++i) {
        auto value = this->make_value(i);

        EXPECT_TRUE(this->queue->try_enqueue(std::move(value)));
    }

    for (size_t i = 0; i < TEST_SIZE; ++i) {
        auto value = this->make_value(i);
        const std::optional<ValueType> dequeued_value = this->queue->try_dequeue();

        EXPECT_TRUE(this->are_equal(std::move(value), std::move(dequeued_value)));
    }
}

TYPED_TEST(BoundedQueueTest, FullQueueConsistent) {
    using ValueType = multithreading::utilities::tests::QueueValueType<TypeParam>::type;
    for (size_t i = 0; i < DEFAULT_QUEUE_SIZE; ++i) {
        auto value = this->make_value(i);

        EXPECT_TRUE(this->queue->try_enqueue(std::move(value)));
    }

    auto value = this->make_value(-1);
    EXPECT_FALSE(this->queue->try_enqueue(std::move(value)));

    for (size_t i = 0; i < DEFAULT_QUEUE_SIZE; ++i) {
        value = this->make_value(i);
        const std::optional<ValueType> dequeued_value = this->queue->try_dequeue();

        EXPECT_TRUE(this->are_equal(std::move(value), std::move(dequeued_value)));
    }
}

TYPED_TEST(BoundedQueueTest, FreeAndRefill) {
    using ValueType = multithreading::utilities::tests::QueueValueType<TypeParam>::type;

    for (size_t i = 0; i < DEFAULT_QUEUE_SIZE; ++i) {
        auto value = this->make_value(i);

        EXPECT_TRUE(this->queue->try_enqueue(std::move(value)));
    }

    auto value = this->make_value(0);
    const std::optional<ValueType> dequeued_value = this->queue->try_dequeue();
    EXPECT_TRUE(this->are_equal(std::move(dequeued_value.value()), std::move(value)));

    value = this->make_value(DEFAULT_QUEUE_SIZE);
    EXPECT_TRUE(this->queue->try_enqueue(std::move(value)));
    EXPECT_TRUE(this->queue->is_full(false));
}

TYPED_TEST(BoundedQueueTest, WaitDequeueTimeout) {
    constexpr std::chrono::duration TEST_DURATION = std::chrono::seconds(1);

    const auto dequeue_value = this->queue->wait_dequeue(TEST_DURATION);
    EXPECT_FALSE(dequeue_value.has_value());
}

TYPED_TEST(BoundedQueueTest, AsyncWaitDequeueTriggers) {
    using ValueType = multithreading::utilities::tests::QueueValueType<TypeParam>::type;
    constexpr std::chrono::duration TEST_DURATION = std::chrono::seconds(1);

    std::future<std::optional<ValueType>> dequeue_future = this->queue->wait_dequeue_async(TEST_DURATION);
    std::this_thread::sleep_for(TEST_DURATION / 2);

    auto value = this->make_value(0);
    this->queue->try_enqueue(std::move(value));
    const std::optional<ValueType> dequeue_value = dequeue_future.get();

    EXPECT_TRUE(dequeue_value.has_value());
    EXPECT_TRUE(this->queue->is_empty(false));
}

TYPED_TEST(BoundedQueueTest, WaitDequeueTriggers) {
    using ValueType = multithreading::utilities::tests::QueueValueType<TypeParam>::type;
    constexpr std::chrono::duration TEST_DURATION = std::chrono::seconds(1);

    std::optional<ValueType> dequeue_value;
    auto dequeue_thread = std::thread([this, TEST_DURATION, &dequeue_value]() {
        dequeue_value = this->queue->wait_dequeue(TEST_DURATION);
    });

    std::this_thread::sleep_for(TEST_DURATION / 2);
    auto value = this->make_value(0);
    this->queue->try_enqueue(std::move(value));
    dequeue_thread.join();

    EXPECT_TRUE(dequeue_value.has_value());
}

TYPED_TEST(BoundedQueueTest, EnqueueAfterWaitDequeueTimeout) {
    using ValueType = multithreading::utilities::tests::QueueValueType<TypeParam>::type;
    constexpr std::chrono::duration TEST_DURATION = std::chrono::seconds(1);
    constexpr double TIMEOUT_OFFSET = 0.1;

    std::optional<ValueType> dequeue_value;
    auto dequeue_thread = std::thread([this, TEST_DURATION, &dequeue_value]() {
        dequeue_value = this->queue->wait_dequeue(TEST_DURATION);
    });

    std::this_thread::sleep_for(TEST_DURATION * (1 + TIMEOUT_OFFSET));
    auto value = this->make_value(0);
    this->queue->try_enqueue(std::move(value));
    dequeue_thread.join();

    EXPECT_FALSE(dequeue_value.has_value());
}

TYPED_TEST(BoundedQueueTest, HighContentionEnqueueResolution) {
    std::map<int, size_t> elements_occurrences;
    std::mutex occurrences_mutex;
    multithreading::utilities::threads::ThreadBarrier barrier;

    for (size_t i = 0; i < DEFAULT_QUEUE_SIZE; ++i) {
        barrier.enqueue([i, this, &elements_occurrences, &occurrences_mutex]() {
            auto value = this->make_value(i);

            if (this->queue->try_enqueue(std::move(value))) {
                const std::scoped_lock guard(occurrences_mutex);
                if (!elements_occurrences.contains(i)) {
                    elements_occurrences.emplace(i, 1);
                } else {
                    const size_t occurrences = elements_occurrences.at(i);
                    elements_occurrences.at(i) = occurrences + 1;
                }
            }
        });
    }

    barrier.kickstart();
    barrier.join();

    for (size_t i = 0; i < DEFAULT_QUEUE_SIZE; ++i) {
        const int casted_i = static_cast<int>(i);
        const size_t occurrences = elements_occurrences.at(casted_i);

        EXPECT_TRUE(this->queue->try_dequeue().has_value());
        EXPECT_EQ(occurrences, 1);
        elements_occurrences.erase(casted_i);
    }

    EXPECT_EQ(elements_occurrences.size(), 0);
}

TYPED_TEST(BoundedQueueTest, HighContentionDequeueResolution) {
    using ValueType = multithreading::utilities::tests::QueueValueType<TypeParam>::type;
    std::map<int, size_t> elements_occurrences;
    std::mutex occurrences_mutex;
    multithreading::utilities::threads::ThreadBarrier barrier;

    for (size_t i = 0; i < DEFAULT_QUEUE_SIZE; ++i) {
        const int int_key = static_cast<int>(i);
        auto value = this->make_value(int_key);
        EXPECT_TRUE(this->queue->try_enqueue(std::move(value)));

        if (!elements_occurrences.contains(int_key)) {
            elements_occurrences.emplace(int_key, 1);
        } else {
            const size_t occurrences = elements_occurrences.at(int_key);
            elements_occurrences.at(int_key) = occurrences + 1;
        }
    }

    for (size_t i = 0; i < DEFAULT_QUEUE_SIZE; ++i) {
        barrier.enqueue([this, &elements_occurrences, &occurrences_mutex]() {
            if (auto dequeued_value = this->queue->try_dequeue(); dequeued_value.has_value()) {
                const ValueType value = std::move(dequeued_value.value());
                const int raw_value = this->to_raw(std::move(value));
                const std::scoped_lock guard(occurrences_mutex);

                if (elements_occurrences.contains(raw_value) && elements_occurrences.at(raw_value) == 1) {
                    elements_occurrences.erase(raw_value);
                }
            }
        });
    }

    barrier.kickstart();
    barrier.join();

    EXPECT_EQ(elements_occurrences.size(), 0);
    EXPECT_FALSE(this->queue->try_dequeue());
    EXPECT_TRUE(this->queue->is_empty(false));
}

TYPED_TEST(BoundedQueueTest, HighContentionMCMPResolution) {
    constexpr std::chrono::duration TEST_DURATION = std::chrono::seconds(1);
    std::atomic<size_t> enqueue_count = 0;
    std::atomic<size_t> dequeue_count = 0;
    multithreading::utilities::threads::ThreadBarrier enqueue_barrier;
    multithreading::utilities::threads::ThreadBarrier dequeue_barrier;

    for (size_t i = 0; i < DEFAULT_QUEUE_SIZE; ++i) {
        enqueue_barrier.enqueue([i, this, &enqueue_count]() {
            if (auto value = this->make_value(i); this->queue->try_enqueue(std::move(value))) {
                enqueue_count.fetch_add(1, std::memory_order_relaxed);
            }
        });
        dequeue_barrier.enqueue([this, &dequeue_count, TEST_DURATION]() {
            if (this->queue->wait_dequeue(TEST_DURATION).has_value()) {
                dequeue_count.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    enqueue_barrier.kickstart();
    dequeue_barrier.kickstart();

    enqueue_barrier.join();
    dequeue_barrier.join();

    EXPECT_EQ(
        enqueue_count.load(std::memory_order_acquire),
        dequeue_count.load(std::memory_order_acquire)
    );
    EXPECT_TRUE(this->queue->is_empty(false));
}

TYPED_TEST(BoundedQueueTest, SevereContentionMCMPResolution) {
    using ValueType = multithreading::utilities::tests::QueueValueType<TypeParam>::type;
    constexpr size_t LARGE_QUEUE_SIZE = DEFAULT_QUEUE_SIZE * 10;
    constexpr size_t THREADS_COUNT = 8;
    auto large_queue = std::make_unique<TypeParam>(LARGE_QUEUE_SIZE);

    std::atomic<size_t> enqueue_count = 0;
    std::atomic<size_t> dequeue_count = 0;
    multithreading::utilities::threads::ThreadBarrier barrier;

    for (size_t i = 0; i < THREADS_COUNT; ++i) {
        barrier.enqueue([this, &large_queue, i, &enqueue_count]() {
            for (size_t j = 0; j < LARGE_QUEUE_SIZE; ++j) {
                auto value = this->make_value(i);
                while (!large_queue->try_enqueue(std::move(value))) {
                    std::this_thread::yield();
                }

                enqueue_count.fetch_add(1, std::memory_order_relaxed);
            }
        });
        barrier.enqueue([&large_queue, &dequeue_count]() {
            for (size_t j = 0; j < LARGE_QUEUE_SIZE; ++j) {
                while (!large_queue->try_dequeue().has_value()) {
                    std::this_thread::yield();
                }

                dequeue_count.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    barrier.kickstart();
    barrier.join();

    EXPECT_EQ(
        enqueue_count.load(std::memory_order_acquire),
        dequeue_count.load(std::memory_order_acquire)
    );
    EXPECT_TRUE(large_queue->is_empty(false));
}