#include <gtest/gtest.h>

#include "ak/base/base.hpp" // IWYU pragma: keep

using namespace ak;
using namespace ak::priv;

struct Data {
    AkDLink node;
    int     value;
};

TEST(UtlAkDLinkTest, BasicOperations) {
    Data d1{.node = {}, .value = 100};
    Data d2{.node = {}, .value = 200};
    Data d3{.node = {}, .value = 300};

    ak_init_dlink(&d1.node);
    ak_init_dlink(&d2.node);
    ak_init_dlink(&d3.node);

    EXPECT_TRUE(ak_is_dlink_detached(&d1.node));
    EXPECT_TRUE(ak_is_dlink_detached(&d2.node));
    EXPECT_TRUE(ak_is_dlink_detached(&d3.node));

    ak_enqueue_dlink(&d1.node, &d2.node);
    EXPECT_FALSE(ak_is_dlink_detached(&d1.node));
    EXPECT_FALSE(ak_is_dlink_detached(&d2.node));
    EXPECT_EQ(d1.node.next, &d2.node);
    EXPECT_EQ(d1.node.prev, &d2.node);
    EXPECT_EQ(d2.node.prev, &d1.node);
    EXPECT_EQ(d2.node.next, &d1.node);

    ak_enqueue_dlink(&d2.node, &d3.node);
    EXPECT_EQ(d3.node.prev, &d2.node);
    EXPECT_EQ(d3.node.next, &d1.node);
    EXPECT_EQ(d2.node.next, &d3.node);
    EXPECT_EQ(d1.node.prev, &d3.node);
}