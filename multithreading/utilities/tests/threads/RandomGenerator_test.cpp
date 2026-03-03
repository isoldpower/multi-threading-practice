#include <gtest/gtest.h>

#include "../../include/threads/RandomGenerator.h"

using multithreading::utilities::threads::generate_random_double;
using multithreading::utilities::threads::generate_random_size_t;

constexpr size_t SMALL_GENERATE_ATTEMPTS = 100;
constexpr size_t GENERATE_ATTEMPTS = 10000;
constexpr size_t EXCESSIVE_GENERATE_ATTEMPTS = 100000;


TEST(RandomUtilsTest, GenerateRandomDoubleIsInUnitRange) {
    for (size_t i = 0; i < GENERATE_ATTEMPTS; ++i) {
        const double value = generate_random_double();
        EXPECT_GE(value, 0.0);
        EXPECT_LE(value, 1.0);
    }
}

TEST(RandomUtilsTest, GenerateRandomDoubleWithRangeIsWithinBounds) {
    constexpr double MIN_RANGE = -5.5;
    constexpr double MAX_RANGE = 10.3;

    for (size_t i = 0; i < GENERATE_ATTEMPTS; ++i) {
        const double value = generate_random_double(MIN_RANGE, MAX_RANGE);
        EXPECT_GE(value, MIN_RANGE);
        EXPECT_LE(value, MAX_RANGE);
    }
}

TEST(RandomUtilsTest, GenerateRandomSizeTIsWithinBounds) {
    constexpr size_t MIN_RANGE = 3;
    constexpr size_t MAX_RANGE = 10;

    for (size_t i = 0; i < GENERATE_ATTEMPTS; ++i) {
        const size_t value = generate_random_size_t(MIN_RANGE, MAX_RANGE);
        EXPECT_GE(value, MIN_RANGE);
        EXPECT_LE(value, MAX_RANGE);
    }
}

TEST(RandomUtilsTest, GenerateRandomDoubleCoversRange) {
    double min_seen = std::numeric_limits<double>::max();
    double max_seen = std::numeric_limits<double>::lowest();

    for (size_t i = 0; i < GENERATE_ATTEMPTS; ++i) {
        const double value = generate_random_double();
        min_seen = std::min(min_seen, value);
        max_seen = std::max(max_seen, value);
    }

    EXPECT_LT(min_seen, 0.1);
    EXPECT_GT(max_seen, 0.9);
}

TEST(RandomUtilsTest, GenerateRandomSizeTCoversAllValues) {
    constexpr size_t MIN = 0;
    constexpr size_t MAX = 9;
    std::array<bool, 10> visited{};
    visited.fill(false);

    for (size_t i = 0; i < GENERATE_ATTEMPTS; ++i) {
        visited.at(generate_random_size_t(MIN, MAX)) = true;
    }

    for (size_t value = MIN; value <= MAX; ++value) {
        EXPECT_TRUE(visited.at(value)) << "Value " << value << " was never generated";
    }
}

TEST(RandomUtilsTest, GenerateRandomDoubleIsRoughlyUniform) {
    constexpr size_t BUCKETS = 10;
    constexpr double ALLOWED_DEVIATION = 0.2;

    std::array<size_t, BUCKETS> counts{};
    counts.fill(0);

    for (size_t i = 0; i < EXCESSIVE_GENERATE_ATTEMPTS; ++i) {
        const double value = generate_random_double();
        const size_t bucket = std::min(static_cast<size_t>(value * BUCKETS), BUCKETS - 1);
        counts.at(bucket)++;
    }

    constexpr double expected = static_cast<double>(EXCESSIVE_GENERATE_ATTEMPTS) / BUCKETS;
    for (size_t i = 0; i < BUCKETS; ++i) {
        // Allow (ALLOWED_DEVIATION*100)% deviation from expected uniform distribution
        EXPECT_NEAR(static_cast<double>(counts.at(i)), expected, expected * ALLOWED_DEVIATION)
            << "Bucket " << i << " is not uniformly distributed";
    }
}

TEST(RandomUtilsTest, DifferentThreadsProduceDifferentSequences) {
    std::vector<double> main_thread_values(SMALL_GENERATE_ATTEMPTS);
    std::vector<double> other_thread_values(SMALL_GENERATE_ATTEMPTS);

    for (size_t i = 0; i < SMALL_GENERATE_ATTEMPTS; ++i) {
        main_thread_values.at(i) = generate_random_double();
    }

    std::thread thread([&other_thread_values]() {
        for (size_t i = 0; i < SMALL_GENERATE_ATTEMPTS; ++i) {
            other_thread_values.at(i) = generate_random_double();
        }
    });
    thread.join();

    EXPECT_NE(main_thread_values, other_thread_values);
}

TEST(RandomUtilsTest, ConcurrentCallsAreSafe) {
    constexpr size_t THREAD_COUNT = 8;
    std::vector<std::thread> threads;
    std::vector<std::string> failures(THREAD_COUNT);
    threads.reserve(THREAD_COUNT);

    for (size_t i = 0; i < THREAD_COUNT; ++i) {
        threads.emplace_back([&failures, i]() {
            for (size_t j = 0; j < GENERATE_ATTEMPTS; ++j) {
                const double unranged_double = generate_random_double();
                const double ranged_double = generate_random_double(-100.0, 100.0);
                const size_t ranged_sizet = generate_random_size_t(0, 100);

                if (unranged_double < 0.0 || unranged_double > 1.0) {
                    failures.at(i) += "unranged_double out of [0, 1] at j=" + std::to_string(j) + "\n";
                }
                if (ranged_double < -100.0 || ranged_double > 100.0) {
                    failures.at(i) += "ranged_double out of [-100, 100] at j=" + std::to_string(j) + "\n";
                }
                if (ranged_sizet > 100UL) {
                    failures.at(i) += "ranged_sizet out of [0, 100] at j=" + std::to_string(j) + "\n";
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

TEST(RandomUtilsTest, GenerateRandomSizeTWithEqualMinMaxReturnsMin) {
    constexpr size_t VALUE = 5;

    for (size_t i = 0; i < SMALL_GENERATE_ATTEMPTS; ++i) {
        EXPECT_EQ(generate_random_size_t(VALUE, VALUE), VALUE);
    }
}

TEST(RandomUtilsTest, GenerateRandomDoubleWithEqualMinMaxReturnsMin) {
    constexpr double VALUE = 3.14;

    for (size_t i = 0; i < SMALL_GENERATE_ATTEMPTS; ++i) {
        EXPECT_DOUBLE_EQ(generate_random_double(VALUE, VALUE), VALUE);
    }
}