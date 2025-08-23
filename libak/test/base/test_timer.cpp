#include <gtest/gtest.h>
#include <unistd.h>
#include "ak/base/base_api.hpp" // IWYU pragma: keep
using namespace ak;

TEST(TimerTest, ReadTimer) {
    using namespace ak;
    U64 t1 = query_timer_ns();
    usleep(200);
    U64 t2 = query_timer_ns();

    EXPECT_GT(t2, t1);
    std::print("{} microseconds\n", (t2 - t1) / 1000);
}