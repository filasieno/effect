#include "ak.hpp" // IWYU pragma: keep
#include <gtest/gtest.h>

using namespace ak;
using namespace ak::priv;

static inline void reset_mask(U64* mask) { *mask = 0ull; }
static inline U32  bin_of(Size s) { if (s==0) return 0; U64 b = (U64)((s-1) >> 5); return (U32)(b > 63 ? 63 : b); }

TEST(AllocFreelistMaskTest, IndexingAndMaskOps) {
    alignas(64) U64 m = 0;

    EXPECT_EQ(bin_of(1), 0);
    EXPECT_EQ(bin_of(32), 0);
    EXPECT_EQ(bin_of(33), 1);
    EXPECT_EQ(bin_of(2048), 63);

    reset_mask(&m);
    EXPECT_EQ(find_alloc_freelist_index(&m, 1), -1);
    EXPECT_EQ(find_alloc_freelist_index(&m, 2048), -1);

    reset_mask(&m); set_alloc_freelist_mask(&m, 0);  EXPECT_EQ(find_alloc_freelist_index(&m, 32), 0);
    reset_mask(&m); set_alloc_freelist_mask(&m, 1);  EXPECT_EQ(find_alloc_freelist_index(&m, 33), 1);
    reset_mask(&m); set_alloc_freelist_mask(&m, 10); EXPECT_EQ(find_alloc_freelist_index(&m, 321), 10);
    reset_mask(&m); set_alloc_freelist_mask(&m, 62); EXPECT_EQ(find_alloc_freelist_index(&m, 2016), 62);
    reset_mask(&m); set_alloc_freelist_mask(&m, 63); EXPECT_EQ(find_alloc_freelist_index(&m, 2000), 63);

    reset_mask(&m); set_alloc_freelist_mask(&m, 5); set_alloc_freelist_mask(&m, 7);
    EXPECT_EQ(find_alloc_freelist_index(&m, (5*32)+1), 5);
    EXPECT_EQ(find_alloc_freelist_index(&m, (6*32)+1), 7);

    reset_mask(&m); set_alloc_freelist_mask(&m, 0); set_alloc_freelist_mask(&m, 1);
    clear_alloc_freelist_mask(&m, 0);
    EXPECT_FALSE(get_alloc_freelist_mask(&m, 0));
    EXPECT_TRUE(get_alloc_freelist_mask(&m, 1));
    EXPECT_EQ(find_alloc_freelist_index(&m, 1), 1);

    reset_mask(&m); set_alloc_freelist_mask(&m, 63);
    EXPECT_EQ(find_alloc_freelist_index(&m, 1u<<30), -1); // > 2048 handled by large-tree path
    EXPECT_EQ(find_alloc_freelist_index(&m, 0), 63);

    reset_mask(&m);
    for (int i = 0; i <= 10; ++i) set_alloc_freelist_mask(&m, i);
    clear_alloc_freelist_mask(&m, 2);
    clear_alloc_freelist_mask(&m, 4);
    EXPECT_EQ(find_alloc_freelist_index(&m, 1), 0);
    EXPECT_EQ(find_alloc_freelist_index(&m, 65), 3);

    reset_mask(&m); for (int i = 0; i < 64; ++i) set_alloc_freelist_mask(&m, i);
    EXPECT_EQ(find_alloc_freelist_index(&m, (64*32)+1), -1);
}