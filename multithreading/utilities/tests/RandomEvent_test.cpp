#include <gtest/gtest.h>

#include "../include/RandomEvent.h"

using multithreading::utilities::random_event;

TEST(RandomEventTest, AlwaysCallsFirstWhenChanceIsOne) {
    constexpr size_t ITERATIONS = 1000;
    size_t first_count = 0;

    for (size_t i = 0; i < ITERATIONS; ++i) {
        random_event<void>(1.0,
            [&first_count]() { ++first_count; },
            []() {}
        );
    }

    EXPECT_EQ(first_count, ITERATIONS);
}

TEST(RandomEventTest, AlwaysCallsSecondWhenChanceIsZero) {
    constexpr size_t ITERATIONS = 1000;
    size_t second_count = 0;

    for (size_t i = 0; i < ITERATIONS; ++i) {
        random_event<void>(
            0.0,
            []() {},
            [&second_count]() { ++second_count; }
        );
    }

    EXPECT_EQ(second_count, ITERATIONS);
}

TEST(RandomEventTest, ReturnsFirstBranchValue) {
    const int result = random_event<int>(
        1.0,
        []() {
            return 42;
        },
        []() {
            return 99;
        }
    );

    EXPECT_EQ(result, 42);
}

TEST(RandomEventTest, ReturnsSecondBranchValue) {
    const int result = random_event<int>(
        0.0,
        []() {
            return 42;
        },
        []() {
            return 99;
        }
    );

    EXPECT_EQ(result, 99);
}

TEST(RandomEventTest, ExactlyOneBranchIsCalledPerInvocation) {
    constexpr size_t ITERATIONS = 10000;

    for (size_t i = 0; i < ITERATIONS; ++i) {
        size_t call_count = 0;
        random_event<void>(
            0.5,
            [&call_count]() {
                ++call_count;
            },
            [&call_count]() {
                ++call_count;
            }
        );

        EXPECT_EQ(call_count, 1) << "Expected exactly one branch called at iteration " << i;
    }
}

TEST(RandomEventTest, DistributionMatchesChance) {
    constexpr size_t ITERATIONS = 100000;
    constexpr double CHANCE = 0.7;
    constexpr double TOLERANCE = 0.02;
    size_t first_count = 0;

    for (size_t i = 0; i < ITERATIONS; ++i) {
        random_event<void>(CHANCE,
            [&first_count]() {
                ++first_count;
            },
            []() {}
        );
    }

    const double actual_ratio = static_cast<double>(first_count) / ITERATIONS;
    EXPECT_NEAR(actual_ratio, CHANCE, TOLERANCE);
}

TEST(RandomEventTest, WorksWithStringReturnType) {
    const auto result = random_event<std::string>(
        1.0,
        []() {
            return std::string("first");
        },
        []() {
            return std::string("second");
        }
    );

    EXPECT_EQ(result, "first");
}

TEST(RandomEventTest, WorksWithUniquePtr) {
    auto result = random_event<std::unique_ptr<int>>(
        1.0,
        []() {
            return std::make_unique<int>(1);
        },
        []() {
            return std::make_unique<int>(2);
        }
    );

    EXPECT_EQ(*result, 1);
}

TEST(RandomEventTest, ConcurrentCallsAreSafe) {
    constexpr size_t THREAD_COUNT = 8;
    constexpr size_t ITERATIONS = 10000;
    std::vector<std::thread> threads;
    std::vector<std::string> failures(THREAD_COUNT);
    threads.reserve(THREAD_COUNT);

    for (size_t i = 0; i < THREAD_COUNT; ++i) {
        threads.emplace_back([&failures, i]() {
            for (size_t j = 0; j < ITERATIONS; ++j) {
                size_t call_count = 0;
                random_event<void>(
                    0.5,
                    [&call_count]() {
                        ++call_count;
                    },
                    [&call_count]() {
                        ++call_count;
                    }
                );
                if (call_count != 1) {
                    failures.at(i) += "Expected exactly one branch at j=" + std::to_string(j) + "\n";
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    for (size_t i = 0; i < THREAD_COUNT; ++i) {
        EXPECT_TRUE(failures.at(i).empty()) << "Thread " << i << " failures:\n" << failures.at(i);
    }
}