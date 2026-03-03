#include <gtest/gtest.h>
#include <vector>

#include "../../include/tests/AmorphicTest.h"

using multithreading::utilities::tests::AmorphicTest;

using AmorphicTestTypes = ::testing::Types<
    std::vector<int>,
    std::vector<std::unique_ptr<int>>
>;

TYPED_TEST_SUITE(AmorphicTest, AmorphicTestTypes);

TYPED_TEST(AmorphicTest, MakeValueProducesCorrectRawValue) {
    const auto value = this->make_value(42);
    EXPECT_EQ(this->to_raw(value), 42);
}

TYPED_TEST(AmorphicTest, MakeValueProducesDistinctValues) {
    const auto a = this->make_value(1);
    const auto b = this->make_value(2);
    EXPECT_NE(this->to_raw(a), this->to_raw(b));
}

TYPED_TEST(AmorphicTest, AreEqualRawTrueForSameValue) {
    const auto lhs = this->make_value(7);
    const auto rhs = this->make_value(7);
    EXPECT_TRUE(this->are_equal_raw(lhs, rhs));
}

TYPED_TEST(AmorphicTest, AreEqualRawFalseForDifferentValues) {
    const auto lhs = this->make_value(1);
    const auto rhs = this->make_value(2);
    EXPECT_FALSE(this->are_equal_raw(lhs, rhs));
}

TYPED_TEST(AmorphicTest, AreEqualTrueWhenOptionalHasMatchingValue) {
    const auto value = this->make_value(5);
    std::optional<typename TestFixture::ValueType> opt = this->make_value(5);
    EXPECT_TRUE(this->are_equal(value, opt));
}

TYPED_TEST(AmorphicTest, AreEqualFalseWhenOptionalIsEmpty) {
    const auto value = this->make_value(5);
    EXPECT_FALSE(this->are_equal(value, std::nullopt));
}

TYPED_TEST(AmorphicTest, AreEqualFalseWhenOptionalHasDifferentValue) {
    const auto value = this->make_value(5);
    std::optional<typename TestFixture::ValueType> opt = this->make_value(99);
    EXPECT_FALSE(this->are_equal(value, opt));
}

TYPED_TEST(AmorphicTest, ToRawExtractsCorrectValue) {
    const auto value = this->make_value(13);
    EXPECT_EQ(this->to_raw(value), 13);
}

TYPED_TEST(AmorphicTest, ToRawReflectsMakeValueInput) {
    for (int i = -5; i <= 5; ++i) {
        const auto value = this->make_value(i);
        EXPECT_EQ(this->to_raw(value), i);
    }
}