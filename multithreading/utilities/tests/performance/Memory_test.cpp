#include "../../include/performance/Memory.h"

#include <gtest/gtest.h>
#include <sys/mman.h>

#include <thread>


using multithreading::utilities::performance::MemoryMeasurement;

namespace {
    constexpr double MIN_PLAUSIBLE_MEMORY_KiB = 1.0;
    constexpr double MAX_PLAUSIBLE_MEMORY_KiB = 10.0 * 1024.0 * 1024.0;

    constexpr size_t SMALL_ALLOC_SIZE  = 16 * 1024 * 1024;
    constexpr size_t MEDIUM_ALLOC_SIZE = 64 * 1024 * 1024;
    constexpr size_t LARGE_ALLOC_SIZE  = 128 * 1024 * 1024;

    constexpr size_t STABILITY_ITERATIONS  = 100;
    constexpr size_t PERFORMANCE_ITERATIONS = 1000;
    constexpr long MAX_MEASUREMENT_MS = 1000;

    constexpr size_t CONCURRENT_THREAD_COUNT = 8;

    constexpr double CONSECUTIVE_TOLERANCE_KiB = 1024.0;
} // anonymous namespace


TEST(MemoryMeasurementTest, PeakMemoryUsageIsPositive) {
    const double peak = MemoryMeasurement::peakMemoryUsage();
    EXPECT_GT(peak, 0.0);
}

TEST(MemoryMeasurementTest, CurrentMemoryUsageIsPositive) {
    const double current = MemoryMeasurement::currentMemoryUsage();
    EXPECT_GT(current, 0.0);
}

TEST(MemoryMeasurementTest, ValuesAreWithinPlausibleRange) {
    const double current = MemoryMeasurement::currentMemoryUsage();
    EXPECT_GT(current, MIN_PLAUSIBLE_MEMORY_KiB);
    EXPECT_LT(current, MAX_PLAUSIBLE_MEMORY_KiB);
}

TEST(MemoryMeasurementTest, CurrentUsageIncreasesAfterAllocation) {
    const double before = MemoryMeasurement::currentMemoryUsage();

    constexpr size_t ALLOC_SIZE = MEDIUM_ALLOC_SIZE;
    auto* block = new char[ALLOC_SIZE];
    std::fill(block, block + ALLOC_SIZE, 1);

    const double after = MemoryMeasurement::currentMemoryUsage();
    delete[] block;

    EXPECT_GT(after, before);
}

TEST(MemoryMeasurementTest, PeakNeverDecreasesAfterFree) {
    const double peak_before = MemoryMeasurement::peakMemoryUsage();

    {
        constexpr size_t ALLOC_SIZE = MEDIUM_ALLOC_SIZE;
        auto* block = new char[ALLOC_SIZE];
        std::fill(block, block + ALLOC_SIZE, 1);
        delete[] block;
    }

    const double peak_after = MemoryMeasurement::peakMemoryUsage();
    EXPECT_GE(peak_after, peak_before);
}

TEST(MemoryMeasurementTest, PeakIsAlwaysAtLeastCurrent) {
    const double current = MemoryMeasurement::currentMemoryUsage();
    const double peak = MemoryMeasurement::peakMemoryUsage();
    EXPECT_GE(peak, current);
}

TEST(MemoryMeasurementTest, RepeatedCallsAreStable) {
    for (size_t i = 0; i < STABILITY_ITERATIONS; ++i) {
        EXPECT_GT(MemoryMeasurement::currentMemoryUsage(), 0.0);
        EXPECT_GT(MemoryMeasurement::peakMemoryUsage(), 0.0);
    }
}

TEST(MemoryMeasurementTest, ConcurrentCallsAreSafe) {
    std::vector<double> results(CONCURRENT_THREAD_COUNT, 0.0);
    std::vector<std::thread> threads;
    threads.reserve(CONCURRENT_THREAD_COUNT);

    for (size_t i = 0; i < CONCURRENT_THREAD_COUNT; ++i) {
        threads.emplace_back([&results, i]() {
            results.at(i) = MemoryMeasurement::currentMemoryUsage();
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }
    for (const double result : results) {
        EXPECT_GT(result, 0.0);
    }
}

TEST(MemoryMeasurementTest, LargerAllocationReportsHigherUsage) {
    constexpr size_t SMALL_ALLOC = SMALL_ALLOC_SIZE;
    constexpr size_t LARGE_ALLOC = LARGE_ALLOC_SIZE;

    auto* small_block = new char[SMALL_ALLOC];
    std::fill(small_block, small_block + SMALL_ALLOC, 1);
    const double small_usage = MemoryMeasurement::currentMemoryUsage();

    auto* large_block = new char[LARGE_ALLOC];
    std::fill(large_block, large_block + LARGE_ALLOC, 1);
    const double large_usage = MemoryMeasurement::currentMemoryUsage();

    delete[] small_block;
    delete[] large_block;

    EXPECT_GT(large_usage, small_usage);
}

TEST(MemoryMeasurementTest, MeasurementCompletesWithinReasonableTime) {
    const auto start = std::chrono::steady_clock::now();

    for (size_t i = 0; i < PERFORMANCE_ITERATIONS; ++i) {
        MemoryMeasurement::currentMemoryUsage();
    }

    const auto elapsed = std::chrono::steady_clock::now() - start;
    const auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

    EXPECT_LT(duration_ms, MAX_MEASUREMENT_MS);
}

TEST(MemoryMeasurementTest, ConsecutiveCallsWithoutAllocationAreConsistent) {
    const double first = MemoryMeasurement::currentMemoryUsage();
    const double second = MemoryMeasurement::currentMemoryUsage();

    EXPECT_NEAR(first, second, CONSECUTIVE_TOLERANCE_KiB);
}