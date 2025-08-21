#include "ak.hpp" // IWYU pragma: keep
#include <gtest/gtest.h>

using namespace ak;
using namespace ak::priv;

struct Data {
    priv::DLink node;
    int        value;
};

TEST(UtlDLinkTest, BasicOperations) {
    Data d1{.node = {}, .value = 100};
    Data d2{.node = {}, .value = 200};
    Data d3{.node = {}, .value = 300};

    init_dlink(&d1.node);
    init_dlink(&d2.node);
    init_dlink(&d3.node);

    EXPECT_TRUE(is_dlink_detached(&d1.node));
    EXPECT_TRUE(is_dlink_detached(&d2.node));
    EXPECT_TRUE(is_dlink_detached(&d3.node));

    enqueue_dlink(&d1.node, &d2.node);
    EXPECT_FALSE(is_dlink_detached(&d1.node));
    EXPECT_FALSE(is_dlink_detached(&d2.node));
    EXPECT_EQ(d1.node.next, &d2.node);
    EXPECT_EQ(d1.node.prev, &d2.node);
    EXPECT_EQ(d2.node.prev, &d1.node);
    EXPECT_EQ(d2.node.next, &d1.node);

    enqueue_dlink(&d2.node, &d3.node);
    EXPECT_EQ(d3.node.prev, &d2.node);
    EXPECT_EQ(d3.node.next, &d1.node);
    EXPECT_EQ(d2.node.next, &d3.node);
    EXPECT_EQ(d1.node.prev, &d3.node);
}