#include <gtest/gtest.h>
#include <multithreading/utilities/include/tests/AmorphicTest.h>
#include <multithreading/utilities/include/threads/ThreadBarrier.h>

#include <memory>

#include "../../include/unbounded_queue/FGLockUnboundedQueue.h"
#include "../../include/unbounded_queue/LockFreeUnboundedQueue.h"


constexpr size_t TEST_SAMPLE_SIZE = 10;

template <typename T>
class UnboundedQueueTest : public multithreading::utilities::tests::AmorphicTest<T> {
protected:
    std::unique_ptr<T> queue;

    void SetUp() override {
        queue = std::make_unique<T>();
    }

    void TearDown() override {
        queue = nullptr;
    }
};

using UnboundedQueueImplementations = ::testing::Types<
    multithreading::structures::unbounded_queue::FGLockUnboundedQueue<int>,
    multithreading::structures::unbounded_queue::FGLockUnboundedQueue<std::unique_ptr<int>>,
    multithreading::structures::unbounded_queue::LockFreeUnboundedQueue<int>,
    multithreading::structures::unbounded_queue::LockFreeUnboundedQueue<std::unique_ptr<int>>
>;
TYPED_TEST_SUITE(UnboundedQueueTest, UnboundedQueueImplementations);

TYPED_TEST(UnboundedQueueTest, StartsEmpty) {
    EXPECT_TRUE(this->queue->is_empty());
}

TYPED_TEST(UnboundedQueueTest, EmptyDequeueNullopt) {
    using ValueType = multithreading::utilities::tests::QueueValueType<TypeParam>::type;
    const std::optional<ValueType> dequeued_value = this->queue->try_dequeue();
    EXPECT_FALSE(dequeued_value.has_value());
}

TYPED_TEST(UnboundedQueueTest, SizeReflectsEnqueue) {
    auto value = this->make_value(1);
    this->queue->enqueue(std::move(value));

    EXPECT_FALSE(this->queue->is_empty());
}

TYPED_TEST(UnboundedQueueTest, DequeueNotEmpty) {
    using ValueType = multithreading::utilities::tests::QueueValueType<TypeParam>::type;

    auto value = this->make_value(1);
    this->queue->enqueue(std::move(value));

    const std::optional<ValueType> dequeued_value = this->queue->try_dequeue();
    EXPECT_TRUE(dequeued_value.has_value());
}

TYPED_TEST(UnboundedQueueTest, FifoOperationsOrder) {
    using ValueType = multithreading::utilities::tests::QueueValueType<TypeParam>::type;
    constexpr size_t TEST_SIZE = 2;

    for (size_t i = 0; i < TEST_SIZE; ++i) {
        auto value = this->make_value(i);
        this->queue->enqueue(std::move(value));
    }

    for (size_t i = 0; i < TEST_SIZE; ++i) {
        const std::optional<ValueType> dequeued_value = this->queue->try_dequeue();
        auto value = this->make_value(i);
        EXPECT_TRUE(this->are_equal(std::move(value), std::move(dequeued_value)));
    }
}

TYPED_TEST(UnboundedQueueTest, FreeAndRefill) {
    using ValueType = multithreading::utilities::tests::QueueValueType<TypeParam>::type;

    for (size_t i = 0; i < TEST_SAMPLE_SIZE; ++i) {
        auto value = this->make_value(i);
        this->queue->enqueue(std::move(value));
    }

    std::optional<ValueType> dequeued_value = this->queue->try_dequeue();
    auto value = this->make_value(0);
    EXPECT_TRUE(this->are_equal(std::move(value), std::move(dequeued_value)));

    value = this->make_value(TEST_SAMPLE_SIZE);
    this->queue->enqueue(std::move(value));
    for (size_t i = 1; i < TEST_SAMPLE_SIZE; ++i) {
        dequeued_value = this->queue->try_dequeue();
        value = this->make_value(i);
        EXPECT_TRUE(this->are_equal(std::move(value), std::move(dequeued_value)));
    }

    dequeued_value = this->queue->try_dequeue();
    value = this->make_value(TEST_SAMPLE_SIZE);
    EXPECT_TRUE(this->are_equal(std::move(value), std::move(dequeued_value)));
}

TYPED_TEST(UnboundedQueueTest, WaitDequeueTimeout) {
    constexpr std::chrono::duration TEST_DURATION = std::chrono::seconds(1);

    const auto dequeue_value = this->queue->wait_dequeue(TEST_DURATION);
    EXPECT_FALSE(dequeue_value.has_value());
}

TYPED_TEST(UnboundedQueueTest, AsyncWaitDequeueTriggers) {
    using ValueType = multithreading::utilities::tests::QueueValueType<TypeParam>::type;
    constexpr std::chrono::duration TEST_DURATION = std::chrono::seconds(1);

    std::future<std::optional<ValueType>> dequeue_future = this->queue->wait_dequeue_async(TEST_DURATION);
    std::this_thread::sleep_for(TEST_DURATION / 2);
    auto value = this->make_value(0);
    this->queue->enqueue(std::move(value));
    const std::optional<ValueType> dequeue_value = dequeue_future.get();

    EXPECT_TRUE(dequeue_value.has_value());
    EXPECT_TRUE(this->queue->is_empty());
}

TYPED_TEST(UnboundedQueueTest, WaitDequeueTriggers) {
    using ValueType = multithreading::utilities::tests::QueueValueType<TypeParam>::type;
    constexpr std::chrono::duration TEST_DURATION = std::chrono::seconds(1);

    std::optional<ValueType> dequeue_value;
    auto dequeue_thread = std::thread([this, TEST_DURATION, &dequeue_value]() {
        dequeue_value = this->queue->wait_dequeue(TEST_DURATION);
    });

    std::this_thread::sleep_for(TEST_DURATION / 2);
    auto value = this->make_value(0);
    this->queue->enqueue(std::move(value));
    dequeue_thread.join();

    EXPECT_TRUE(dequeue_value.has_value());
}

TYPED_TEST(UnboundedQueueTest, EnqueueAfterWaitDequeueTimeout) {
    using ValueType = multithreading::utilities::tests::QueueValueType<TypeParam>::type;
    constexpr std::chrono::duration TEST_DURATION = std::chrono::seconds(1);
    constexpr double TIMEOUT_OFFSET = 0.1;

    std::optional<ValueType> dequeue_value;
    auto dequeue_thread = std::thread([this, TEST_DURATION, &dequeue_value]() {
        dequeue_value = this->queue->wait_dequeue(TEST_DURATION);
    });

    std::this_thread::sleep_for(TEST_DURATION * (1 + TIMEOUT_OFFSET));
    auto value = this->make_value(0);
    this->queue->enqueue(std::move(value));
    dequeue_thread.join();

    EXPECT_FALSE(dequeue_value.has_value());
}

TYPED_TEST(UnboundedQueueTest, HighContentionEnqueueResolution) {
    std::map<int, size_t> elements_occurrences;
    std::mutex occurrences_mutex;
    multithreading::utilities::threads::ThreadBarrier barrier;

    for (size_t i = 0; i < TEST_SAMPLE_SIZE; ++i) {
        barrier.enqueue([i, this, &elements_occurrences, &occurrences_mutex]() {
            auto value = this->make_value(i);
            this->queue->enqueue(std::move(value));

            const std::scoped_lock guard(occurrences_mutex);
            if (!elements_occurrences.contains(i)) {
                elements_occurrences.emplace(i, 1);
            } else {
                const size_t occurrences = elements_occurrences.at(i);
                elements_occurrences.at(i) = occurrences + 1;
            }
        });
    }

    barrier.kickstart();
    barrier.join();

    for (size_t i = 0; i < TEST_SAMPLE_SIZE; ++i) {
        const int casted_i = static_cast<int>(i);
        const size_t occurrences = elements_occurrences.at(casted_i);

        EXPECT_TRUE(this->queue->try_dequeue().has_value());
        EXPECT_EQ(occurrences, 1);
        elements_occurrences.erase(casted_i);
    }

    EXPECT_EQ(elements_occurrences.size(), 0);
}

TYPED_TEST(UnboundedQueueTest, HighContentionDequeueResolution) {
    using ValueType = multithreading::utilities::tests::QueueValueType<TypeParam>::type;
    std::map<int, size_t> elements_occurrences;
    std::mutex occurrences_mutex;
    multithreading::utilities::threads::ThreadBarrier barrier;

    for (size_t i = 0; i < TEST_SAMPLE_SIZE; ++i) {
        const int int_key = static_cast<int>(i);
        auto value = this->make_value(int_key);
        this->queue->enqueue(std::move(value));

        if (!elements_occurrences.contains(int_key)) {
            elements_occurrences.emplace(int_key, 1);
        } else {
            const size_t occurrences = elements_occurrences.at(int_key);
            elements_occurrences.at(int_key) = occurrences + 1;
        }
    }

    for (size_t i = 0; i < TEST_SAMPLE_SIZE; ++i) {
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
    EXPECT_TRUE(this->queue->is_empty());
}

TYPED_TEST(UnboundedQueueTest, HighContentionMCMPResolution) {
    constexpr std::chrono::duration TEST_DURATION = std::chrono::seconds(1);
    std::atomic<size_t> enqueue_count = 0;
    std::atomic<size_t> dequeue_count = 0;
    multithreading::utilities::threads::ThreadBarrier enqueue_barrier;
    multithreading::utilities::threads::ThreadBarrier dequeue_barrier;

    for (size_t i = 0; i < TEST_SAMPLE_SIZE; ++i) {
        enqueue_barrier.enqueue([i, this, &enqueue_count]() {
            auto value = this->make_value(i);
            this->queue->enqueue(std::move(value));
            enqueue_count.fetch_add(1, std::memory_order_relaxed);
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
    EXPECT_TRUE(this->queue->is_empty());
}

TYPED_TEST(UnboundedQueueTest, SevereContentionMCMPResolution) {
    constexpr std::chrono::duration TEST_DURATION = std::chrono::milliseconds(100);
    constexpr size_t LARGE_QUEUE_SIZE = TEST_SAMPLE_SIZE * 10;
    constexpr size_t THREADS_COUNT = 8;

    std::atomic<size_t> enqueue_count = 0;
    std::atomic<size_t> dequeue_count = 0;
    multithreading::utilities::threads::ThreadBarrier barrier;

    for (size_t i = 0; i < THREADS_COUNT; ++i) {
        barrier.enqueue([this, &enqueue_count]() {
            for (size_t j = 0; j < LARGE_QUEUE_SIZE; ++j) {
                auto value = this->make_value(j);
                this->queue->enqueue(std::move(value));
                enqueue_count.fetch_add(1, std::memory_order_relaxed);
            }
        });
        barrier.enqueue([this, &dequeue_count, TEST_DURATION]() {
            for (size_t j = 0; j < LARGE_QUEUE_SIZE; ++j) {
                if (this->queue->wait_dequeue(TEST_DURATION).has_value()) {
                    dequeue_count.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    barrier.kickstart();
    barrier.join();

    EXPECT_EQ(
        enqueue_count.load(std::memory_order_acquire),
        dequeue_count.load(std::memory_order_acquire)
    );
    EXPECT_TRUE(this->queue->is_empty());
}