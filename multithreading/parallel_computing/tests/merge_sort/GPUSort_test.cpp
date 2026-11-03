#include <gtest/gtest.h>
#include <parallel_computing/merge_sort/GPUBitonicSort.h>
#include <parallel_computing/merge_sort/GPUMergeSort.h>
#include <parallel_computing/merge_sort/GPUSortContext.h>
#include <parallel_computing/merge_sort/MergeSort.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <random>
#include <span>
#include <stdexcept>
#include <thread>
#include <vector>

using multithreading::parallel_computing::merge_sort::GPUBitonicSort;
using multithreading::parallel_computing::merge_sort::GPUMergeSort;
using multithreading::parallel_computing::merge_sort::GPUSortContext;
using multithreading::parallel_computing::merge_sort::MergeSort;

/** Created once for the whole test binary: loading the kernels compiles GPU pipelines. */
static std::shared_ptr<const GPUSortContext> shared_context() {
    static const std::shared_ptr<const GPUSortContext> context =
        GPUSortContext::create(MT_GPU_SORT_LIBRARY);
    return context;
}

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

namespace {
    struct BitonicSorter {
        template <typename T, typename Compare = std::less<T>>
        static std::unique_ptr<MergeSort<T, Compare>> make() {
            return std::make_unique<GPUBitonicSort<T, Compare>>(shared_context());
        }
    };

    struct GPUMergeSorter {
        template <typename T, typename Compare = std::less<T>>
        static std::unique_ptr<MergeSort<T, Compare>> make() {
            return std::make_unique<GPUMergeSort<T, Compare>>(shared_context());
        }
    };

    template <typename Sorter>
    class GPUSortTest : public ::testing::Test {
    protected:
        void SetUp() override {
            if (!shared_context()) {
                GTEST_SKIP() << "No supported GPU on this machine";
            }
            sorter = Sorter::template make<int>();
        }

        std::unique_ptr<MergeSort<int>> sorter;

        void expect_sorts_like_std(std::vector<int> values) {
            std::vector<int> expected = values;
            std::ranges::sort(expected);
            sorter->perform_sort(values);
            EXPECT_EQ(values, expected);
        }
    };
}  // namespace

using GPUSortImplementations = ::testing::Types<BitonicSorter, GPUMergeSorter>;
TYPED_TEST_SUITE(GPUSortTest, GPUSortImplementations);

TYPED_TEST(GPUSortTest, EmptyRangeStaysEmpty) {
    std::vector<int> values;
    this->sorter->perform_sort(values);
    EXPECT_TRUE(values.empty());
}

TYPED_TEST(GPUSortTest, SingleElementUnchanged) {
    this->expect_sorts_like_std({42});
}

TYPED_TEST(GPUSortTest, TwoElementsSwapped) {
    this->expect_sorts_like_std({2, 1});
}

TYPED_TEST(GPUSortTest, AlreadySortedUnchanged) {
    this->expect_sorts_like_std({1, 2, 3, 4, 5, 6, 7, 8, 9});
}

TYPED_TEST(GPUSortTest, ReversedBecomesAscending) {
    this->expect_sorts_like_std({9, 8, 7, 6, 5, 4, 3, 2, 1, 0});
}

TYPED_TEST(GPUSortTest, AllEqualUnchanged) {
    this->expect_sorts_like_std(std::vector<int>(33, 7));
}

TYPED_TEST(GPUSortTest, DuplicatesAndNegatives) {
    this->expect_sorts_like_std({3, -1, 3, 0, -1, 2, 2, -5, 3});
}

TYPED_TEST(GPUSortTest, ExtremeValuesSurvivePadding) {
    constexpr int max = std::numeric_limits<int>::max();
    constexpr int min = std::numeric_limits<int>::min();
    this->expect_sorts_like_std({max, 0, min, max, -1, min, max});
}

TYPED_TEST(GPUSortTest, RandomSizesAroundPowersOfTwo) {
    for (const std::size_t size : {3U, 7U, 15U, 16U, 17U, 1023U, 1024U, 1025U, 2047U, 2048U, 2049U, 4097U, 65537U}) {
        SCOPED_TRACE(size);
        this->expect_sorts_like_std(random_values(size));
    }
}

TYPED_TEST(GPUSortTest, LargeRandomMatchesStd) {
    this->expect_sorts_like_std(random_values(1'000'003));
}

TYPED_TEST(GPUSortTest, SortsOnlyTheGivenSubrange) {
    std::vector<int> values{9, 5, 4, 3, 2, 1, 0};
    this->sorter->perform_sort(std::span<int>(values).subspan(1, 4));
    EXPECT_EQ(values, (std::vector<int>{9, 2, 3, 4, 5, 1, 0}));
}

TYPED_TEST(GPUSortTest, GreaterSortsDescending) {
    auto descending = TypeParam::template make<int, std::greater<>>();
    std::vector<int> values = random_values(5000);
    std::vector<int> expected = values;
    std::ranges::sort(expected, std::greater<>{});

    descending->perform_sort(values);
    EXPECT_EQ(values, expected);
}

TYPED_TEST(GPUSortTest, UnsignedValues) {
    auto sorter = TypeParam::template make<std::uint32_t>();
    std::vector<std::uint32_t> values{4'000'000'000U, 0U, 7U, std::numeric_limits<std::uint32_t>::max(), 7U};
    std::vector<std::uint32_t> expected = values;
    std::ranges::sort(expected);

    sorter->perform_sort(values);
    EXPECT_EQ(values, expected);
}

TYPED_TEST(GPUSortTest, FloatValuesIncludingInfinities) {
    auto ascending = TypeParam::template make<float>();
    auto descending = TypeParam::template make<float, std::greater<float>>();
    constexpr float infinity = std::numeric_limits<float>::infinity();
    const std::vector<float> input{2.5F, -infinity, 0.0F, -3.25F, infinity, 1e-7F, -0.5F};

    std::vector<float> values = input;
    std::vector<float> expected = input;
    std::ranges::sort(expected);
    ascending->perform_sort(values);
    EXPECT_EQ(values, expected);

    values = input;
    std::ranges::sort(expected, std::greater<>{});
    descending->perform_sort(values);
    EXPECT_EQ(values, expected);
}

TYPED_TEST(GPUSortTest, OneInstanceSortsSeveralRangesAtOnce) {
    constexpr std::size_t CALLERS = 4;

    std::vector<std::vector<int>> inputs;
    std::vector<std::vector<int>> expected;
    for (std::size_t i = 0; i < CALLERS; i++) {
        inputs.push_back(random_values(20'000 + i));
        expected.push_back(inputs.back());
        std::ranges::sort(expected.back());
    }

    std::vector<std::thread> callers;
    callers.reserve(CALLERS);
    for (auto& input : inputs) {
        callers.emplace_back([this, &input]() {
            this->sorter->perform_sort(input);
        });
    }
    for (auto& caller : callers) {
        caller.join();
    }

    EXPECT_EQ(inputs, expected);
}

TEST(GPUSortContextTest, RejectsMissingDevice) {
    EXPECT_THROW(GPUSortContext(nullptr), std::invalid_argument);
}

TEST(GPUSortContextTest, SortersRejectMissingContext) {
    EXPECT_THROW(GPUBitonicSort<int>(nullptr), std::invalid_argument);
    EXPECT_THROW(GPUMergeSort<int>(nullptr), std::invalid_argument);
}
