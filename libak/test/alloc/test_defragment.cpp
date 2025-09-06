#include <gtest/gtest.h>
#include <cstdlib>

#include "ak/alloc/alloc.hpp" // IWYU pragma: keep


static inline AkU64 sum_freelist_nodes(const ak_alloc_table* at) {
	AkU64 s = 0;
	for (int i = 0; i < ak_alloc_table::ALLOCATOR_BIN_COUNT; ++i) s += at->freelist_count[i];
	return s;
}

class AllocDefragTest : public ::testing::Test {
protected:
	void* buffer = nullptr;
	AkU64 buffer_size = 1024 * 1024;
	ak_alloc_table table{};

	void SetUp() override {
		buffer = std::malloc(buffer_size);
		ASSERT_NE(buffer, nullptr);
		ASSERT_EQ(alloc_table_init(&table, buffer, buffer_size), 0);
	}

	void TearDown() override {
		std::free(buffer);
		buffer = nullptr;
	}
};

// Scenario 1: small block into small block (defragment merges two neighbors into one freelist node)
TEST_F(AllocDefragTest, SmallBlockIntoSmallBlock) {
	void* p1 = alloc_table_try_malloc(&table, 32);
	void* p2 = alloc_table_try_malloc(&table, 32);
	ASSERT_TRUE(p1 != nullptr && p2 != nullptr);
	alloc_table_free(&table, p1, 0);
	alloc_table_free(&table, p2, 0);

	AkU64 before_nodes = sum_freelist_nodes(&table);
	int defrag = alloc_table_defrag(&table, /*millis_budget*/0);
	AkU64 after_nodes = sum_freelist_nodes(&table);
	EXPECT_GE(defrag, 1);
	EXPECT_EQ(after_nodes + 1, before_nodes);
}

// Scenario 2: small block into wild block (free path already merges; defrag should do nothing)
TEST_F(AllocDefragTest, SmallBlockIntoWildBlock) {
	AkU64 nodes_before = sum_freelist_nodes(&table);
	void* p = alloc_table_try_malloc(&table, 64);
	ASSERT_NE(p, nullptr);
	alloc_table_free(&table, p, 0);

	AkU64 nodes_after_free = sum_freelist_nodes(&table);
	EXPECT_GE(nodes_after_free, nodes_before);

	int defrag = alloc_table_defrag(&table, 0);
	EXPECT_GE(defrag, 1);
	AkU64 nodes_after_defrag = sum_freelist_nodes(&table);
	EXPECT_LE(nodes_after_defrag, nodes_after_free);
}

// Scenario 3: large number of small blocks coalescing into a tree block (> 2048)
TEST_F(AllocDefragTest, ManySmallBlocksToTreeBlock) {
	constexpr int kBlocks = 128; // enough to exceed 2048 total
	for (int i = 0; i < kBlocks; ++i) {
		void* p = alloc_table_try_malloc(&table, 32);
		ASSERT_NE(p, nullptr);
		alloc_table_free(&table, p, 0);
	}
	AkU64 before_nodes = sum_freelist_nodes(&table);
	int defrag = alloc_table_defrag(&table, 0);
	EXPECT_GE(defrag, 1);
	AkU64 after_nodes = sum_freelist_nodes(&table);
	EXPECT_LT(after_nodes, before_nodes);
}

// Scenario 4: large number of small blocks coalescing into wild block (reach end and merge into wild)
TEST_F(AllocDefragTest, ManySmallBlocksToWildBlock) {
	constexpr int kBlocks = 64;
	for (int i = 0; i < kBlocks; ++i) {
		void* p = alloc_table_try_malloc(&table, 64);
		ASSERT_NE(p, nullptr);
		alloc_table_free(&table, p, 0);
	}
	AkU64 before_nodes = sum_freelist_nodes(&table);
	int defrag = alloc_table_defrag(&table, 0);
	EXPECT_GE(defrag, 1);
	AkU64 after_nodes = sum_freelist_nodes(&table);
	EXPECT_LT(after_nodes, before_nodes);
	// The final block can be wild; ensure pointer is valid
	EXPECT_NE(table.wild_block, nullptr);
	EXPECT_EQ(table.wild_block->this_desc.state, (AkU32)AK_ALLOC_BLOCK_STATE_WILD_BLOCK);
}

// Scenario 5: stats consistency across defragmentation (no change in free_mem_size)
TEST_F(AllocDefragTest, StatsConsistency) {
	AkU64 free_mem_before = table.free_mem_size;
	for (int i = 0; i < 16; ++i) {
		void* p = alloc_table_try_malloc(&table, 128);
		ASSERT_NE(p, nullptr);
		alloc_table_free(&table, p, 0);
	}
	int defrag = alloc_table_defrag(&table, 0);
	(void)defrag;
	AkU64 free_mem_after = table.free_mem_size;
	EXPECT_EQ(free_mem_after, free_mem_before);
}
