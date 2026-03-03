#include <gtest/gtest.h>

#include "../../include/performance/AlignedField.h"


TEST(AlignedFieldTest, DefaultAlignmentIs64) {
    multithreading::utilities::performance::AlignedField<int> field{};

    EXPECT_EQ(alignof(decltype(field)), 64);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(&field.value) % 64, 0);
}

TEST(AlignedFieldTest, CustomAlignment) {
    constexpr size_t ALIGNMENT = 128;

    multithreading::utilities::performance::AlignedField<int, ALIGNMENT> field{};
    EXPECT_EQ(reinterpret_cast<uintptr_t>(&field.value) % ALIGNMENT, 0);
}

TEST(AlignedFieldTest, SizeIsMultipleOfAlignment) {
    EXPECT_EQ(sizeof(multithreading::utilities::performance::AlignedField<int>) % 64, 0);
    EXPECT_EQ(sizeof(multithreading::utilities::performance::AlignedField<int, 128>) % 128, 0);
}

TEST(AlignedFieldTest, AdjacentFieldsDoNotShareCacheLine) {
    multithreading::utilities::performance::AlignedField<int> first{}, second{};
    const auto addr_first = reinterpret_cast<uintptr_t>(&first);
    const auto addr_second = reinterpret_cast<uintptr_t>(&second);

    EXPECT_GE(std::abs(static_cast<ptrdiff_t>(addr_second - addr_first)), 64);
}

TEST(AlignedFieldTest, DefaultConstruction) {
    EXPECT_NO_THROW(multithreading::utilities::performance::AlignedField<int> field;);
}

TEST(AlignedFieldTest, ValueConstruction) {
    constexpr int FIELD_VALUE = 42;
    multithreading::utilities::performance::AlignedField<int> field(FIELD_VALUE);
    EXPECT_EQ(*field, FIELD_VALUE);
}

TEST(AlignedFieldTest, InPlaceConstruction) {
    multithreading::utilities::performance::AlignedField<std::pair<int,int>> field(std::in_place, 1, 2);

    EXPECT_EQ(field->first, 1);
    EXPECT_EQ(field->second, 2);
}

TEST(AlignedFieldTest, MoveOnlyType) {
    constexpr int FIELD_VALUE = 99;
    multithreading::utilities::performance::AlignedField<std::unique_ptr<int>> field(
        std::make_unique<int>(FIELD_VALUE));
    EXPECT_EQ(**field, FIELD_VALUE);
}

TEST(AlignedFieldTest, DereferenceOperator) {
    multithreading::utilities::performance::AlignedField<int> field(7);
    EXPECT_EQ(*field, 7);
    *field = 42;
    EXPECT_EQ(*field, 42);
}

TEST(AlignedFieldTest, ArrowOperator) {
    multithreading::utilities::performance::AlignedField<std::vector<int>> field;
    field->push_back(1);
    EXPECT_EQ(field->size(), 1);
}

TEST(AlignedFieldTest, ImplicitConversion) {
    multithreading::utilities::performance::AlignedField<int> field(5);
    int& ref = field;
    ref = 10;
    EXPECT_EQ(*field, 10);
}

TEST(AlignedFieldTest, ConstAccess) {
    const multithreading::utilities::performance::AlignedField<int> field(3);
    EXPECT_EQ(*field, 3);
    const int& ref = field;
    EXPECT_EQ(ref, 3);
}

TEST(AlignedFieldTest, AtomicFieldIsAligned) {
    multithreading::utilities::performance::AlignedField<std::atomic<int*>> field;
    EXPECT_EQ(reinterpret_cast<uintptr_t>(&field.value) % 64, 0);

    int x = 42;
    field->store(&x, std::memory_order_relaxed);
    EXPECT_EQ(*field->load(std::memory_order_relaxed), 42);
}