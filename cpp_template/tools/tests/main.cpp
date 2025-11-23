#include <gtest/gtest.h>

TEST(Group1, Test1) {
  EXPECT_EQ(3, 1 + 2);
}

TEST(Group1, Test2) {
  ASSERT_FLOAT_EQ(3.0F, 1.0F + 2.0F);
}