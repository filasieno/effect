#include <gtest/gtest.h>

#include "ak/base/base.hpp" // IWYU pragma: keep

struct Data {
    struct ak_dlink node;
    int             value;
};

TEST(AkDLinkTest, BasicOperations) {
    Data d1{.node = {}, .value = 100};
    Data d2{.node = {}, .value = 200};
    Data d3{.node = {}, .value = 300};

    ak_dlink_init(&d1.node);
    ak_dlink_init(&d2.node);
    ak_dlink_init(&d3.node);

    EXPECT_TRUE(ak_dlink_is_detached(&d1.node));
    EXPECT_TRUE(ak_dlink_is_detached(&d2.node));
    EXPECT_TRUE(ak_dlink_is_detached(&d3.node));

    ak_dlink_enqueue(&d1.node, &d2.node);
    EXPECT_FALSE(ak_dlink_is_detached(&d1.node));
    EXPECT_FALSE(ak_dlink_is_detached(&d2.node));
    EXPECT_EQ(d1.node.next, &d2.node);
    EXPECT_EQ(d1.node.prev, &d2.node);
    EXPECT_EQ(d2.node.prev, &d1.node);
    EXPECT_EQ(d2.node.next, &d1.node);

    ak_dlink_enqueue(&d2.node, &d3.node);
    EXPECT_EQ(d3.node.prev, &d2.node);
    EXPECT_EQ(d3.node.next, &d1.node);
    EXPECT_EQ(d2.node.next, &d3.node);
    EXPECT_EQ(d1.node.prev, &d3.node);
}