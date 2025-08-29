#include <gtest/gtest.h>
#include "ak/alloc/alloc.hpp" // IWYU pragma: keep

using namespace ak;
using namespace ak::priv;

static inline void reset_mask(U64* mask) { *mask = 0ull; }
static inline U32  bin_of(Size s) { if (s==0) return 0; U64 b = (U64)((s-1) >> 5); return (U32)(b > 63 ? 63 : b); }

TEST(AllocFreelistMaskTest, IndexingAndMaskOps) {
    alignas(64) U64 m = 0;

    EXPECT_EQ(bin_of(1u), 0u);
    EXPECT_EQ(bin_of(32u), 0u);
    EXPECT_EQ(bin_of(33u), 1u);
    EXPECT_EQ(bin_of(2048u), 63u);

    reset_mask(&m);
    EXPECT_EQ(find_alloc_freelist_index(&m, 1u), -1);
    EXPECT_EQ(find_alloc_freelist_index(&m, 2048u), -1);

    reset_mask(&m); set_alloc_freelist_mask(&m, 0u);  EXPECT_EQ(find_alloc_freelist_index(&m, 32u), 0);
    reset_mask(&m); set_alloc_freelist_mask(&m, 1u);  EXPECT_EQ(find_alloc_freelist_index(&m, 33u), 1);
    reset_mask(&m); set_alloc_freelist_mask(&m, 10u); EXPECT_EQ(find_alloc_freelist_index(&m, 321u), 10);
    reset_mask(&m); set_alloc_freelist_mask(&m, 62u); EXPECT_EQ(find_alloc_freelist_index(&m, 2016u), 62);
    reset_mask(&m); set_alloc_freelist_mask(&m, 63u); EXPECT_EQ(find_alloc_freelist_index(&m, 2000u), 63);

    reset_mask(&m); set_alloc_freelist_mask(&m, 5u); set_alloc_freelist_mask(&m, 7u);
    EXPECT_EQ(find_alloc_freelist_index(&m, (5u*32u)+1u), 5);
    EXPECT_EQ(find_alloc_freelist_index(&m, (6u*32u)+1u), 7);

    reset_mask(&m); set_alloc_freelist_mask(&m, 0u); set_alloc_freelist_mask(&m, 1u);
    clear_alloc_freelist_mask(&m, 0);
    EXPECT_FALSE(get_alloc_freelist_mask(&m, 0u));
    EXPECT_TRUE(get_alloc_freelist_mask(&m, 1u));
    EXPECT_EQ(find_alloc_freelist_index(&m, 1u), 1);

    reset_mask(&m); set_alloc_freelist_mask(&m, 63u);
    EXPECT_EQ(find_alloc_freelist_index(&m, 1u<<30), -1); // > 2048 handled by large-tree path
    EXPECT_EQ(find_alloc_freelist_index(&m, 0u), 63);

    reset_mask(&m);
    for (int i = 0; i <= 10; ++i) set_alloc_freelist_mask(&m, i);
    clear_alloc_freelist_mask(&m, 2u);
    clear_alloc_freelist_mask(&m, 4u);
    EXPECT_EQ(find_alloc_freelist_index(&m, 1u), 0);
    EXPECT_EQ(find_alloc_freelist_index(&m, 65u), 3);

    reset_mask(&m); for (int i = 0; i < 64; ++i) set_alloc_freelist_mask(&m, i);
    EXPECT_EQ(find_alloc_freelist_index(&m, (64u*32u)+1u), -1);
}