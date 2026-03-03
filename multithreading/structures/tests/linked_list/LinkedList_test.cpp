#include <gtest/gtest.h>
#include <multithreading/utilities/include/RandomEvent.h>
#include <multithreading/utilities/include/tests/AmorphicTest.h>
#include <multithreading/utilities/include/threads/ThreadBarrier.h>

#include <map>
#include <memory>
#include <optional>

#include "../../include/linked_list/FGLockLinkedList.h"
#include "../../include/linked_list/LockFreeLinkedList.h"

constexpr size_t TEST_SAMPLE_SIZE = 10;
constexpr size_t TEST_THREADS_COUNT = 8;

template <typename T>
class LinkedListTest : public multithreading::utilities::tests::AmorphicTest<T> {
protected:
    std::unique_ptr<T> linked_list;

    void SetUp() override {
        linked_list = std::make_unique<T>();
    }

    void TearDown() override {
        linked_list = nullptr;
    }
};

using LinkedListImplementations = ::testing::Types<
    multithreading::structures::linked_list::FGLockLinkedList<int>,
    multithreading::structures::linked_list::FGLockLinkedList<std::unique_ptr<int>>,
    multithreading::structures::linked_list::LockFreeLinkedList<int>,
    multithreading::structures::linked_list::LockFreeLinkedList<std::unique_ptr<int>>
>;
TYPED_TEST_SUITE(LinkedListTest, LinkedListImplementations);

TYPED_TEST(LinkedListTest, StartsZeroSize) {
    EXPECT_EQ(this->linked_list->size(), 0);
}

TYPED_TEST(LinkedListTest, StartsEmpty) {
    EXPECT_EQ(this->linked_list->empty(), true);
}

TYPED_TEST(LinkedListTest, EmptyPopNullopt) {
    const auto pop_value = this->linked_list->pop_front();
    EXPECT_FALSE(pop_value.has_value());
}

TYPED_TEST(LinkedListTest, SizeReflectsPushedElement) {
    auto value = this->make_value(1);
    this->linked_list->push_front(std::move(value));

    EXPECT_EQ(this->linked_list->size(), 1);
}

TYPED_TEST(LinkedListTest, PopDeletesElement) {
    auto value = this->make_value(1);
    this->linked_list->push_front(std::move(value));

    EXPECT_EQ(this->linked_list->size(), 1);

    this->linked_list->pop_front();
    EXPECT_EQ(this->linked_list->size(), 0);
}

TYPED_TEST(LinkedListTest, PopAtCorrectIndex) {
    using ValueType = multithreading::utilities::tests::QueueValueType<TypeParam>::type;

    auto value = this->make_value(1);
    this->linked_list->push_front(std::move(value));
    value = this->make_value(2);
    this->linked_list->push_front(std::move(value));
    value = this->make_value(3);
    this->linked_list->push_front(std::move(value));

    const std::optional<ValueType> pop_value = this->linked_list->pop_at(1);
    EXPECT_TRUE(pop_value.has_value());

    value = this->make_value(2);
    EXPECT_TRUE(this->are_equal(std::move(value), std::move(pop_value)));
}

TYPED_TEST(LinkedListTest, PopEmptyNullopt) {
    using ValueType = multithreading::utilities::tests::QueueValueType<TypeParam>::type;

    std::optional<ValueType> pop_value = this->linked_list->pop_front();
    EXPECT_FALSE(pop_value.has_value());
    pop_value = this->linked_list->pop_back();
    EXPECT_FALSE(pop_value.has_value());
    pop_value = this->linked_list->pop_at(0);
    EXPECT_FALSE(pop_value.has_value());
}

TYPED_TEST(LinkedListTest, PushPopElement) {
    auto value = this->make_value(1);

    this->linked_list->push_front(std::move(value));
    EXPECT_EQ(this->linked_list->size(), 1);

    const auto pop_value = this->linked_list->pop_front();
    EXPECT_TRUE(pop_value.has_value());

    value = this->make_value(1);
    EXPECT_TRUE(this->are_equal(std::move(value), std::move(pop_value)));
}

TYPED_TEST(LinkedListTest, FrontBackCorrectPushOrder) {
    using ValueType = multithreading::utilities::tests::QueueValueType<TypeParam>::type;

    auto value = this->make_value(1);
    this->linked_list->push_front(std::move(value));
    value = this->make_value(2);
    this->linked_list->push_front(std::move(value));
    value = this->make_value(3);
    this->linked_list->push_front(std::move(value));

    std::optional<ValueType> pop_value;
    EXPECT_EQ(this->linked_list->size(), 3);

    pop_value = this->linked_list->pop_front();
    EXPECT_TRUE(pop_value.has_value());
    value = this->make_value(3);
    EXPECT_TRUE(this->are_equal(std::move(value), std::move(pop_value)));

    pop_value = this->linked_list->pop_back();
    EXPECT_TRUE(pop_value.has_value());
    value = this->make_value(1);
    EXPECT_TRUE(this->are_equal(std::move(value), std::move(pop_value)));

    pop_value = this->linked_list->pop_front();
    EXPECT_TRUE(pop_value.has_value());
    value = this->make_value(2);
    EXPECT_TRUE(this->are_equal(std::move(value), std::move(pop_value)));
}

TYPED_TEST(LinkedListTest, ContainsFalseOnEmpty) {
    auto value = this->make_value(0);
    EXPECT_FALSE(this->linked_list->contains(std::move(value)));
}

TYPED_TEST(LinkedListTest, ContainsTrueAfterPush) {
    using ValueType = multithreading::utilities::tests::QueueValueType<TypeParam>::type;

    auto value = this->make_value(1);
    this->linked_list->push_front(std::move(value));

    value = this->make_value(1);
    EXPECT_TRUE(this->linked_list->contains(
        value,
        [this](const ValueType& first, const ValueType& second) {
            return this->are_equal_raw(first, second);
        }
    ));

    value = this->make_value(2);
    EXPECT_FALSE(this->linked_list->contains(
        value,
        [this](const ValueType& first, const ValueType& second) {
            return this->are_equal_raw(first, second);
        }
    ));
}

TYPED_TEST(LinkedListTest, FillWithSamples) {
    using ValueType = multithreading::utilities::tests::QueueValueType<TypeParam>::type;

    for (size_t i = 0; i < TEST_SAMPLE_SIZE; ++i) {
        auto value = this->make_value(i);
        this->linked_list->push_front(std::move(value));
    }
    EXPECT_EQ(this->linked_list->size(), TEST_SAMPLE_SIZE);

    for (size_t i = 0; i < TEST_SAMPLE_SIZE; ++i) {
        auto value = this->make_value(i);
        EXPECT_TRUE(this->linked_list->contains(
            value,
            [this](const ValueType& first, const ValueType& second) {
                return this->are_equal_raw(first, second);
            }
        ));

        const auto pop_value = this->linked_list->pop_back();
        EXPECT_TRUE(pop_value.has_value());
        value = this->make_value(i);
        EXPECT_TRUE(this->are_equal(std::move(value), std::move(pop_value)));
    }
    EXPECT_EQ(this->linked_list->size(), 0);
}

TYPED_TEST(LinkedListTest, HighContentionPushFront) {
    multithreading::utilities::threads::ThreadBarrier barrier;
    std::atomic<size_t> enqueue_count = 0;

    for (size_t i = 0; i < TEST_THREADS_COUNT; ++i) {
        barrier.enqueue([this, &enqueue_count]() {
            for (size_t j = 0; j < TEST_SAMPLE_SIZE; ++j) {
                auto value = this->make_value(j);
                this->linked_list->push_front(std::move(value));
                enqueue_count.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    barrier.kickstart();
    barrier.join();

    for (size_t i = 0; i < TEST_SAMPLE_SIZE * TEST_THREADS_COUNT; ++i) {
        this->linked_list->pop_front();
    }

    EXPECT_EQ(
        enqueue_count.load(std::memory_order_acquire),
        TEST_THREADS_COUNT * TEST_SAMPLE_SIZE
    );
    EXPECT_EQ(this->linked_list->size(), 0);
}

TYPED_TEST(LinkedListTest, HighContentionPushBack) {
    multithreading::utilities::threads::ThreadBarrier barrier;
    std::atomic<size_t> enqueue_count = 0;

    for (size_t i = 0; i < TEST_THREADS_COUNT; ++i) {
        barrier.enqueue([this, &enqueue_count]() {
            for (size_t j = 0; j < TEST_SAMPLE_SIZE; ++j) {
                auto value = this->make_value(j);
                this->linked_list->push_back(std::move(value));
                enqueue_count.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    barrier.kickstart();
    barrier.join();

    for (size_t i = 0; i < TEST_SAMPLE_SIZE * TEST_THREADS_COUNT; ++i) {
        this->linked_list->pop_back();
    }

    EXPECT_EQ(
        enqueue_count.load(std::memory_order_acquire),
        TEST_THREADS_COUNT * TEST_SAMPLE_SIZE
    );
    EXPECT_EQ(this->linked_list->size(), 0);
}

TYPED_TEST(LinkedListTest, HighContentionPushOnEnds) {
    constexpr double PUSH_FRONT_CHANCE = 0.5;
    constexpr size_t ADJUSTED_SAMPLE_SIZE = TEST_SAMPLE_SIZE * 2;

    multithreading::utilities::threads::ThreadBarrier barrier;
    std::atomic<size_t> enqueue_count = 0;

    for (size_t i = 0; i < TEST_THREADS_COUNT; ++i) {
        barrier.enqueue([this, &enqueue_count]() {
            for (size_t j = 0; j < ADJUSTED_SAMPLE_SIZE; ++j) {
                auto value = this->make_value(j);
                multithreading::utilities::random_event<void>(
                    PUSH_FRONT_CHANCE,
                    [this, &value]() {
                        this->linked_list->push_front(std::move(value));
                    },
                    [this, &value]() {
                        this->linked_list->push_back(std::move(value));
                    }
                );

                enqueue_count.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    barrier.kickstart();
    barrier.join();

    for (size_t i = 0; i < TEST_THREADS_COUNT * ADJUSTED_SAMPLE_SIZE; ++i) {
        this->linked_list->pop_back();
    }

    EXPECT_EQ(
        enqueue_count.load(std::memory_order_acquire),
        TEST_THREADS_COUNT * ADJUSTED_SAMPLE_SIZE
    );
    EXPECT_EQ(this->linked_list->size(), 0);
}

TYPED_TEST(LinkedListTest, HighContentionPopFront) {
    multithreading::utilities::threads::ThreadBarrier barrier;
    std::atomic<size_t> dequeue_count = 0;

    for (size_t i = 0; i < TEST_SAMPLE_SIZE * TEST_THREADS_COUNT; ++i) {
        auto value = this->make_value(i);
        this->linked_list->push_front(std::move(value));
    }

    for (size_t i = 0; i < TEST_THREADS_COUNT; ++i) {
        barrier.enqueue([this, &dequeue_count]() {
            for (size_t j = 0; j < TEST_SAMPLE_SIZE; ++j) {
                this->linked_list->pop_front();
                dequeue_count.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    barrier.kickstart();
    barrier.join();

    EXPECT_EQ(
        dequeue_count.load(std::memory_order_acquire),
        TEST_THREADS_COUNT * TEST_SAMPLE_SIZE
    );
    EXPECT_EQ(this->linked_list->size(), 0);
}

TYPED_TEST(LinkedListTest, HighContentionPopBack) {
    multithreading::utilities::threads::ThreadBarrier barrier;
    std::atomic<size_t> dequeue_count = 0;

    for (size_t i = 0; i < TEST_SAMPLE_SIZE * TEST_THREADS_COUNT; ++i) {
        auto value = this->make_value(i);
        this->linked_list->push_back(std::move(value));
    }

    for (size_t i = 0; i < TEST_THREADS_COUNT; ++i) {
        barrier.enqueue([this, &dequeue_count]() {
            for (size_t j = 0; j < TEST_SAMPLE_SIZE; ++j) {
                this->linked_list->pop_back();
                dequeue_count.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    barrier.kickstart();
    barrier.join();

    EXPECT_EQ(
        dequeue_count.load(std::memory_order_acquire),
        TEST_THREADS_COUNT * TEST_SAMPLE_SIZE
    );
    EXPECT_EQ(this->linked_list->size(), 0);
}

TYPED_TEST(LinkedListTest, HighContentionPopOnEnds) {
    using ValueType = multithreading::utilities::tests::QueueValueType<TypeParam>::type;
    constexpr double POP_FRONT_CHANCE = 0.5;
    constexpr size_t ADJUSTED_SAMPLE_SIZE = TEST_SAMPLE_SIZE * 2;

    multithreading::utilities::threads::ThreadBarrier barrier;
    std::atomic<size_t> dequeue_count = 0;
    std::atomic<size_t> remaining = TEST_THREADS_COUNT * ADJUSTED_SAMPLE_SIZE;

    for (size_t i = 0; i < TEST_THREADS_COUNT * ADJUSTED_SAMPLE_SIZE; ++i) {
        auto value = this->make_value(i);
        this->linked_list->push_back(std::move(value));
    }

    for (size_t i = 0; i < TEST_THREADS_COUNT; ++i) {
        barrier.enqueue([this, &dequeue_count, &remaining]() {
            while (true) {
                if (remaining.load(std::memory_order_acquire) == 0) {
                    break;
                }

                const auto result = multithreading::utilities::random_event<std::optional<ValueType>>(
                    POP_FRONT_CHANCE,
                    [this]() {
                        return this->linked_list->pop_front();
                    },
                    [this]() {
                        return this->linked_list->pop_back();
                    }
                );

                if (result.has_value()) {
                    dequeue_count.fetch_add(1, std::memory_order_relaxed);
                    remaining.fetch_sub(1, std::memory_order_release);
                } else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
        });
    }

    barrier.kickstart();
    barrier.join();

    EXPECT_EQ(dequeue_count.load(std::memory_order_acquire), TEST_THREADS_COUNT * ADJUSTED_SAMPLE_SIZE);
    EXPECT_EQ(this->linked_list->size(), 0);
}

TYPED_TEST(LinkedListTest, HighContentionMCMPFront) {
    multithreading::utilities::threads::ThreadBarrier barrier;
    std::atomic<size_t> enqueue_count = 0;
    std::atomic<size_t> dequeue_count = 0;
    std::atomic<size_t> remaining = TEST_THREADS_COUNT * TEST_SAMPLE_SIZE;

    for (size_t i = 0; i < TEST_THREADS_COUNT; ++i) {
        barrier.enqueue([this, &enqueue_count, i]() {
            for (size_t j = 0; j < TEST_SAMPLE_SIZE; ++j) {
                auto value = this->make_value(i * TEST_SAMPLE_SIZE + j);
                this->linked_list->push_front(std::move(value));
                enqueue_count.fetch_add(1, std::memory_order_relaxed);
            }
        });
        barrier.enqueue([this, &dequeue_count, &remaining]() {
            while (true) {
                size_t left = remaining.load(std::memory_order_acquire);
                if (left == 0) {
                    break;
                }

                if (this->linked_list->pop_front().has_value()) {
                    dequeue_count.fetch_add(1, std::memory_order_relaxed);
                    remaining.fetch_sub(1, std::memory_order_release);
                } else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
        });
    }

    barrier.kickstart();
    barrier.join();

    EXPECT_EQ(enqueue_count.load(std::memory_order_acquire), TEST_THREADS_COUNT * TEST_SAMPLE_SIZE);
    EXPECT_EQ(dequeue_count.load(std::memory_order_acquire), TEST_THREADS_COUNT * TEST_SAMPLE_SIZE);
    EXPECT_EQ(this->linked_list->size(), 0);
}

TYPED_TEST(LinkedListTest, HighContentionMCMPBack) {
    multithreading::utilities::threads::ThreadBarrier barrier;
    std::atomic<size_t> enqueue_count = 0;
    std::atomic<size_t> dequeue_count = 0;
    std::atomic<size_t> remaining = TEST_THREADS_COUNT * TEST_SAMPLE_SIZE;

    for (size_t i = 0; i < TEST_THREADS_COUNT; ++i) {
        barrier.enqueue([this, &enqueue_count, i]() {
            for (size_t j = 0; j < TEST_SAMPLE_SIZE; ++j) {
                auto value = this->make_value(i * TEST_SAMPLE_SIZE + j);
                this->linked_list->push_back(std::move(value));
                enqueue_count.fetch_add(1, std::memory_order_relaxed);
            }
        });
        barrier.enqueue([this, &dequeue_count, &remaining]() {
            while (true) {
                size_t left = remaining.load(std::memory_order_acquire);
                if (left == 0) {
                    break;
                }

                if (this->linked_list->pop_back().has_value()) {
                    dequeue_count.fetch_add(1, std::memory_order_relaxed);
                    remaining.fetch_sub(1, std::memory_order_release);
                } else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
        });
    }

    barrier.kickstart();
    barrier.join();

    EXPECT_EQ(enqueue_count.load(std::memory_order_acquire), TEST_THREADS_COUNT * TEST_SAMPLE_SIZE);
    EXPECT_EQ(dequeue_count.load(std::memory_order_acquire), TEST_THREADS_COUNT * TEST_SAMPLE_SIZE);
    EXPECT_EQ(this->linked_list->size(), 0);
}

TYPED_TEST(LinkedListTest, HighContentionMCMPOnEnds) {
    using ValueType = multithreading::utilities::tests::QueueValueType<TypeParam>::type;
    constexpr double PUSH_FRONT_CHANCE = 0.5;

    multithreading::utilities::threads::ThreadBarrier barrier;
    std::atomic<size_t> enqueue_count = 0;
    std::atomic<size_t> dequeue_count = 0;
    std::atomic<size_t> remaining = TEST_THREADS_COUNT * TEST_SAMPLE_SIZE;

    for (size_t i = 0; i < TEST_THREADS_COUNT; ++i) {
        barrier.enqueue([this, &enqueue_count, i]() {
            for (size_t j = 0; j < TEST_SAMPLE_SIZE; ++j) {
                auto value = this->make_value(i * TEST_SAMPLE_SIZE + j);
                multithreading::utilities::random_event<void>(
                    PUSH_FRONT_CHANCE,
                    [this, &value]() {
                        this->linked_list->push_front(std::move(value));
                    },
                    [this, &value]() {
                        this->linked_list->push_back(std::move(value));
                    }
                );
                enqueue_count.fetch_add(1, std::memory_order_relaxed);
            }
        });
        barrier.enqueue([this, &dequeue_count, &remaining]() {
            while (true) {
                 const size_t left = remaining.load(std::memory_order_acquire);
                if (left == 0) {
                    break;
                }

                const auto result = multithreading::utilities::random_event<std::optional<ValueType>>(
                    0.5,
                    [this]() {
                        return this->linked_list->pop_front();
                    },
                    [this]() {
                        return this->linked_list->pop_back();
                    }
                );

                if (result.has_value()) {
                    dequeue_count.fetch_add(1, std::memory_order_relaxed);
                    remaining.fetch_sub(1, std::memory_order_release);
                } else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
        });
    }

    barrier.kickstart();
    barrier.join();

    EXPECT_EQ(enqueue_count.load(std::memory_order_acquire), TEST_THREADS_COUNT * TEST_SAMPLE_SIZE);
    EXPECT_EQ(dequeue_count.load(std::memory_order_acquire), TEST_THREADS_COUNT * TEST_SAMPLE_SIZE);
    EXPECT_EQ(this->linked_list->size(), 0);
}

TYPED_TEST(LinkedListTest, HighContentionMCMPEverywhere) {
    multithreading::utilities::threads::ThreadBarrier barrier;
    std::atomic<size_t> enqueue_count = 0;
    std::atomic<size_t> dequeue_count = 0;
    std::atomic<size_t> remaining = TEST_THREADS_COUNT * TEST_SAMPLE_SIZE;

    for (size_t i = 0; i < TEST_THREADS_COUNT; ++i) {
        barrier.enqueue([this, &enqueue_count, i]() {
            for (size_t j = 0; j < TEST_SAMPLE_SIZE; ++j) {
                const size_t random_index = multithreading::utilities::threads::generate_random_size_t(
                    0,
                    enqueue_count.load(std::memory_order_relaxed)
                );
                auto value = this->make_value((i * TEST_SAMPLE_SIZE) + j);

                if (!this->linked_list->push_at(random_index, std::move(value))) {
                    this->linked_list->push_front(std::move(value));
                }
                enqueue_count.fetch_add(1, std::memory_order_relaxed);
            }
        });
        barrier.enqueue([this, &dequeue_count, &enqueue_count, &remaining]() {
            while (true) {
                if (remaining.load(std::memory_order_acquire) == 0) {
                    break;
                }

                const size_t enqueued = enqueue_count.load(std::memory_order_acquire);
                const size_t dequeued = dequeue_count.load(std::memory_order_acquire);
                const size_t current_size = (enqueued > dequeued) ? (enqueued - dequeued) : 0;

                if (current_size != 0) {
                    const size_t random_index = multithreading::utilities::threads::generate_random_size_t(
                        0,
                        current_size
                    );

                    if (this->linked_list->pop_at(random_index)) {
                        dequeue_count.fetch_add(1, std::memory_order_relaxed);
                        remaining.fetch_sub(1, std::memory_order_release);
                    }
                } else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
        });
    }

    barrier.kickstart();
    barrier.join();

    EXPECT_EQ(enqueue_count.load(std::memory_order_acquire), TEST_THREADS_COUNT * TEST_SAMPLE_SIZE);
    EXPECT_EQ(dequeue_count.load(std::memory_order_acquire), TEST_THREADS_COUNT * TEST_SAMPLE_SIZE);
    EXPECT_EQ(this->linked_list->size(), 0);
}