#include <gtest/gtest.h>
#include <unistd.h>
#include "ak/base/base_api.hpp" // IWYU pragma: keep

TEST(TimerTest, ReadTimer) {
    AkU64 t1 = ak_query_timer_ns();
    usleep(200);
    AkU64 t2 = ak_query_timer_ns();

    EXPECT_GT(t2, t1);
    std::print("{} microseconds\n", (t2 - t1) / 1000);
}