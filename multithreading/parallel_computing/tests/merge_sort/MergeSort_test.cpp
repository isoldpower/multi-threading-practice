#include <gtest/gtest.h>
#include <parallel_computing/merge_sort/MergeSort.h>
#include <parallel_computing/merge_sort/ParallelMergeSort.h>
#include <parallel_computing/merge_sort/SyncMergeSort.h>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <functional>
#include <memory>
#include <random>
#include <span>
#include <stdexcept>
#include <thread>
#include <tuple>
#include <vector>

using multithreading::parallel_computing::merge_sort::MergeSort;
using multithreading::parallel_computing::merge_sort::ParallelMergeSort;
using multithreading::parallel_computing::merge_sort::SyncMergeSort;

/** Values in [-1000, 1000]; the count doubles as the seed, so each size gets its own data. */
static std::vector<int> random_values(const std::size_t count) {
    std::mt19937 generator(static_cast<std::mt19937::result_type>(count));
    std::uniform_int_distribution<int> distribution(-1000, 1000);

    std::vector<int> values(count);
    std::ranges::generate(values, [&]() {
        return distribution(generator);
    });

    return values;
}

static std::vector<int> std_sorted(std::vector<int> values) {
    std::ranges::stable_sort(values);
    return values;
}

namespace {
    struct SyncSorter {
        template <typename T, typename Compare = std::less<T>>
        static std::unique_ptr<MergeSort<T, Compare>> make() {
            return std::make_unique<SyncMergeSort<T, Compare>>();
        }
    };

    /** Forks down to single elements, so even tiny inputs cross threads. */
    template <std::size_t Threads>
    struct ParallelSorter {
        template <typename T, typename Compare = std::less<T>>
        static std::unique_ptr<MergeSort<T, Compare>> make() {
            return std::make_unique<ParallelMergeSort<T, Compare>>(Threads, 1);
        }
    };

    template <typename Sorter>
    class MergeSortTest : public ::testing::Test {
    protected:
        std::unique_ptr<MergeSort<int>> sorter = Sorter::template make<int>();

        void expect_sorts_like_std(std::vector<int> values) {
            const std::vector<int> expected = std_sorted(values);
            sorter->perform_sort(values);
            EXPECT_EQ(values, expected);
        }
    };

    /** Ordered by key alone, so equal keys reveal whether the original order survived. */
    struct KeyedValue {  // NOLINT(altera-struct-pack-align)
        int key = 0;
        std::size_t position = 0;
    };

    constexpr auto by_key = [](const KeyedValue& left, const KeyedValue& right) {
        return left.key < right.key;
    };

    constexpr auto by_pointee = [](const std::unique_ptr<int>& left,
                                   const std::unique_ptr<int>& right) {
        return *left < *right;
    };
}  // namespace

/** Fails the n-th comparison made through any copy, wherever that copy runs. */
static auto throwing_after(const int calls) {
    return
        [calls_left = std::make_shared<std::atomic<int>>(calls)](const int left, const int right) {
            if (calls_left->fetch_sub(1) <= 0) {
                throw std::runtime_error("comparison failed");
            }
            return left < right;
        };
}

using MergeSortImplementations = ::testing::
    Types<SyncSorter, ParallelSorter<1>, ParallelSorter<2>, ParallelSorter<3>, ParallelSorter<8>>;
TYPED_TEST_SUITE(MergeSortTest, MergeSortImplementations);

TYPED_TEST(MergeSortTest, EmptyRangeStaysEmpty) {
    std::vector<int> values;
    this->sorter->perform_sort(values);
    EXPECT_TRUE(values.empty());
}

TYPED_TEST(MergeSortTest, SingleElementUnchanged) {
    this->expect_sorts_like_std({42});
}

TYPED_TEST(MergeSortTest, TwoElementsSwapped) {
    this->expect_sorts_like_std({2, 1});
}

TYPED_TEST(MergeSortTest, AlreadySortedUnchanged) {
    this->expect_sorts_like_std({1, 2, 3, 4, 5, 6, 7, 8, 9});
}

TYPED_TEST(MergeSortTest, ReversedBecomesAscending) {
    this->expect_sorts_like_std({9, 8, 7, 6, 5, 4, 3, 2, 1, 0});
}

TYPED_TEST(MergeSortTest, AllEqualUnchanged) {
    this->expect_sorts_like_std(std::vector<int>(33, 7));
}

TYPED_TEST(MergeSortTest, DuplicatesAndNegatives) {
    this->expect_sorts_like_std({3, -1, 3, 0, -1, 2, 2, -5, 3});
}

TYPED_TEST(MergeSortTest, RandomSizesAroundPowersOfTwo) {
    for (const std::size_t size : {3U, 7U, 15U, 16U, 17U, 1023U, 1024U, 1025U}) {
        SCOPED_TRACE(size);
        this->expect_sorts_like_std(random_values(size));
    }
}

TYPED_TEST(MergeSortTest, LargeRandomMatchesStd) {
    this->expect_sorts_like_std(random_values(200'000));
}

TYPED_TEST(MergeSortTest, SortsOnlyTheGivenSubrange) {
    std::vector<int> values{9, 5, 4, 3, 2, 1, 0};
    this->sorter->perform_sort(std::span<int>(values).subspan(1, 4));
    EXPECT_EQ(values, (std::vector<int>{9, 2, 3, 4, 5, 1, 0}));
}

TYPED_TEST(MergeSortTest, CustomComparatorSortsDescending) {
    auto descending = TypeParam::template make<int, std::greater<>>();
    std::vector<int> values = random_values(5000);
    std::vector<int> expected = values;
    std::ranges::stable_sort(expected, std::greater<>{});

    descending->perform_sort(values);
    EXPECT_EQ(values, expected);
}

TYPED_TEST(MergeSortTest, EqualKeysKeepTheirOrder) {
    auto sorter = TypeParam::template make<KeyedValue, decltype(by_key)>();

    std::vector<KeyedValue> values;
    for (const int value : random_values(10'000)) {
        values.push_back(KeyedValue{.key = value % 10, .position = values.size()});
    }

    sorter->perform_sort(values, by_key);
    // Stable by key means ordered by key first and by original position among equals
    EXPECT_TRUE(std::ranges::is_sorted(values, {}, [](const KeyedValue& value) {
        return std::tie(value.key, value.position);
    }));
}

TYPED_TEST(MergeSortTest, MoveOnlyValuesAreMovedNotCopied) {
    auto sorter = TypeParam::template make<std::unique_ptr<int>, decltype(by_pointee)>();

    std::vector<std::unique_ptr<int>> values;
    for (const int value : random_values(2000)) {
        values.push_back(std::make_unique<int>(value));
    }

    sorter->perform_sort(values, by_pointee);
    ASSERT_TRUE(std::ranges::none_of(values, [](const auto& value) {
        return value == nullptr;
    }));
    EXPECT_TRUE(std::ranges::is_sorted(values, by_pointee));
}

TYPED_TEST(MergeSortTest, ComparatorExceptionReachesCaller) {
    auto compare = throwing_after(1000);
    auto sorter = TypeParam::template make<int, decltype(compare)>();
    std::vector<int> values = random_values(5000);

    EXPECT_THROW(sorter->perform_sort(values, compare), std::runtime_error);
}

TEST(ParallelMergeSortTest, ZeroThreadsRejected) {
    EXPECT_THROW(ParallelMergeSort<int>(0), std::invalid_argument);
}

TEST(ParallelMergeSortTest, DefaultThresholdSortsLargeInput) {
    ParallelMergeSort<int> sorter(std::max(2U, std::thread::hardware_concurrency()));
    std::vector<int> values = random_values(500'000);
    const std::vector<int> expected = std_sorted(values);

    sorter.perform_sort(values);
    EXPECT_EQ(values, expected);
}

TEST(ParallelMergeSortTest, OneInstanceSortsSeveralRangesAtOnce) {
    ParallelMergeSort<int> sorter(4, 1);
    constexpr std::size_t CALLERS = 4;

    std::vector<std::vector<int>> inputs;
    std::vector<std::vector<int>> expected;
    for (std::size_t i = 0; i < CALLERS; i++) {
        inputs.push_back(random_values(20'000 + i));
        expected.push_back(std_sorted(inputs.back()));
    }

    std::vector<std::thread> callers;
    callers.reserve(CALLERS);
    for (auto& input : inputs) {
        callers.emplace_back([&sorter, &input]() {
            sorter.perform_sort(input);
        });
    }
    for (auto& caller : callers) {
        caller.join();
    }

    EXPECT_EQ(inputs, expected);
}
