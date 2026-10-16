#include <gtest/gtest.h>
#include <utilities/performance/AlignedField.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

using multithreading::utilities::performance::AlignedField;
using multithreading::utilities::performance::CACHE_LINE_SIZE;

template <typename T>
static uintptr_t address_of(const T& object) {
    // std::bit_cast would avoid the NOLINT, but clang-tidy 18's static analyzer crashes on
    // arithmetic over a bit_cast pointer value.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<uintptr_t>(&object);
}

TEST(AlignedFieldTest, DefaultAlignmentIsCacheLine) {
    const AlignedField<int> field{};

    EXPECT_EQ(alignof(decltype(field)), CACHE_LINE_SIZE);
    EXPECT_EQ(address_of(field.get()) % CACHE_LINE_SIZE, 0);
}

TEST(AlignedFieldTest, CustomAlignment) {
    constexpr size_t ALIGNMENT = 128;

    const AlignedField<int, ALIGNMENT> field{};
    EXPECT_EQ(address_of(field.get()) % ALIGNMENT, 0);
}

TEST(AlignedFieldTest, SizeIsMultipleOfAlignment) {
    EXPECT_EQ(sizeof(AlignedField<int>) % 64, 0);
    EXPECT_EQ(sizeof(AlignedField<int, 128>) % 128, 0);
}

TEST(AlignedFieldTest, AdjacentFieldsDoNotShareCacheLine) {
    const AlignedField<int> first{};
    const AlignedField<int> second{};
    const uintptr_t addr_first = address_of(first);
    const uintptr_t addr_second = address_of(second);

    EXPECT_GE(std::abs(static_cast<ptrdiff_t>(addr_second - addr_first)), 64);
}

TEST(AlignedFieldTest, DefaultConstructionValueInitialises) {
    const AlignedField<int> field;
    EXPECT_EQ(field.get(), 0);
}

TEST(AlignedFieldTest, ValueConstruction) {
    constexpr int FIELD_VALUE = 42;
    const AlignedField<int> field(FIELD_VALUE);
    EXPECT_EQ(field.get(), FIELD_VALUE);
}

TEST(AlignedFieldTest, InPlaceConstruction) {
    const AlignedField<std::pair<int, int>> field(std::in_place, 1, 2);

    EXPECT_EQ(field.get().first, 1);
    EXPECT_EQ(field.get().second, 2);
}

TEST(AlignedFieldTest, MoveOnlyType) {
    constexpr int FIELD_VALUE = 99;
    const AlignedField<std::unique_ptr<int>> field(std::make_unique<int>(FIELD_VALUE));
    EXPECT_EQ(*field.get(), FIELD_VALUE);
}

TEST(AlignedFieldTest, MutableAccess) {
    AlignedField<int> field(7);
    EXPECT_EQ(field.get(), 7);
    field.get() = 42;
    EXPECT_EQ(field.get(), 42);
}

TEST(AlignedFieldTest, MemberAccessThroughGet) {
    AlignedField<std::vector<int>> field;
    field.get().push_back(1);
    EXPECT_EQ(field.get().size(), 1);
}

TEST(AlignedFieldTest, ReferenceAliasesValue) {
    AlignedField<int> field(5);
    int& ref = field.get();
    ref = 10;
    EXPECT_EQ(field.get(), 10);
}

TEST(AlignedFieldTest, ConstAccess) {
    const AlignedField<int> field(3);
    const int& ref = field.get();
    EXPECT_EQ(ref, 3);
}

TEST(AlignedFieldTest, AtomicFieldIsAligned) {
    AlignedField<std::atomic<int*>> field;
    EXPECT_EQ(address_of(field.get()) % 64, 0);

    int target = 42;
    field.get().store(&target, std::memory_order_relaxed);
    EXPECT_EQ(*field.get().load(std::memory_order_relaxed), 42);
}
