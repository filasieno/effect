#include <gtest/gtest.h>
#include "ak/base/base.hpp" // IWYU pragma: keep

TEST(BasicTest, Equality) {
    EXPECT_EQ(1, 1);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
