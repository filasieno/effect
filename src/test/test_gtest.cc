#define AK_IMPLEMENTATION
#include "ak.hpp" // IWYU pragma: keep
#include <gtest/gtest.h>

TEST(BasicTest, Equality) {
  EXPECT_EQ(1, 1);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  AK_ASSERT(0 == 1);
  return RUN_ALL_TESTS();
}
