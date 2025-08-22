#include <gtest/gtest.h>
#include <cstdlib>

#include "ak/alloc/alloc.hpp" // IWYU pragma: keep

using namespace ak;

static inline U64 sum_freelist_nodes(const AllocTable* at) {
	U64 s = 0;
	for (int i = 0; i < AllocTable::ALLOCATOR_BIN_COUNT; ++i) s += at->freelist_count[i];
	return s;
}

class AllocDefragTest : public ::testing::Test {
protected:
	Void* buffer = nullptr;
	U64 buffer_size = 1024 * 1024;
	AllocTable table{};

	void SetUp() override {
		buffer = std::malloc(buffer_size);
		ASSERT_NE(buffer, nullptr);
		ASSERT_EQ(ak::priv::init_alloc_table(&table, buffer, buffer_size), 0);
	}

	void TearDown() override {
		std::free(buffer);
		buffer = nullptr;
	}
};

// Scenario 1: small block into small block (defragment merges two neighbors into one freelist node)
TEST_F(AllocDefragTest, SmallBlockIntoSmallBlock) {
	Void* p1 = ak::priv::try_alloc_table_malloc(&table, 32);
	Void* p2 = ak::priv::try_alloc_table_malloc(&table, 32);
	ASSERT_TRUE(p1 != nullptr && p2 != nullptr);
	ak::priv::alloc_table_free(&table, p1, 0);
	ak::priv::alloc_table_free(&table, p2, 0);

	U64 before_nodes = sum_freelist_nodes(&table);
	I32 defrag = ak::priv::defrag_alloc_table_mem(&table, /*millis_budget*/0);
	U64 after_nodes = sum_freelist_nodes(&table);
	EXPECT_GE(defrag, 1);
	EXPECT_EQ(after_nodes + 1, before_nodes);
}

// Scenario 2: small block into wild block (free path already merges; defrag should do nothing)
TEST_F(AllocDefragTest, SmallBlockIntoWildBlock) {
	U64 nodes_before = sum_freelist_nodes(&table);
	Void* p = ak::priv::try_alloc_table_malloc(&table, 64);
	ASSERT_NE(p, nullptr);
	ak::priv::alloc_table_free(&table, p, 0);

	U64 nodes_after_free = sum_freelist_nodes(&table);
	EXPECT_GE(nodes_after_free, nodes_before);

	I32 defrag = ak::priv::defrag_alloc_table_mem(&table, 0);
	EXPECT_GE(defrag, 1);
	U64 nodes_after_defrag = sum_freelist_nodes(&table);
	EXPECT_LE(nodes_after_defrag, nodes_after_free);
}

// Scenario 3: large number of small blocks coalescing into a tree block (> 2048)
TEST_F(AllocDefragTest, ManySmallBlocksToTreeBlock) {
	constexpr int kBlocks = 128; // enough to exceed 2048 total
	for (int i = 0; i < kBlocks; ++i) {
		Void* p = ak::priv::try_alloc_table_malloc(&table, 32);
		ASSERT_NE(p, nullptr);
		ak::priv::alloc_table_free(&table, p, 0);
	}
	U64 before_nodes = sum_freelist_nodes(&table);
	I32 defrag = ak::priv::defrag_alloc_table_mem(&table, 0);
	EXPECT_GE(defrag, 1);
	U64 after_nodes = sum_freelist_nodes(&table);
	EXPECT_LT(after_nodes, before_nodes);
}

// Scenario 4: large number of small blocks coalescing into wild block (reach end and merge into wild)
TEST_F(AllocDefragTest, ManySmallBlocksToWildBlock) {
	constexpr int kBlocks = 64;
	for (int i = 0; i < kBlocks; ++i) {
		Void* p = ak::priv::try_alloc_table_malloc(&table, 64);
		ASSERT_NE(p, nullptr);
		ak::priv::alloc_table_free(&table, p, 0);
	}
	U64 before_nodes = sum_freelist_nodes(&table);
	I32 defrag = ak::priv::defrag_alloc_table_mem(&table, 0);
	EXPECT_GE(defrag, 1);
	U64 after_nodes = sum_freelist_nodes(&table);
	EXPECT_LT(after_nodes, before_nodes);
	// The final block can be wild; ensure pointer is valid
	EXPECT_NE(table.wild_block, nullptr);
	EXPECT_EQ(table.wild_block->this_desc.state, (U32)AllocBlockState::WILD_BLOCK);
}

// Scenario 5: stats consistency across defragmentation (no change in free_mem_size)
TEST_F(AllocDefragTest, StatsConsistency) {
	U64 free_mem_before = table.free_mem_size;
	for (int i = 0; i < 16; ++i) {
		Void* p = ak::priv::try_alloc_table_malloc(&table, 128);
		ASSERT_NE(p, nullptr);
		ak::priv::alloc_table_free(&table, p, 0);
	}
	I32 defrag = ak::priv::defrag_alloc_table_mem(&table, 0);
	(void)defrag;
	U64 free_mem_after = table.free_mem_size;
	EXPECT_EQ(free_mem_after, free_mem_before);
}
