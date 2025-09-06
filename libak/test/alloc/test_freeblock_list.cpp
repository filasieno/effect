#include <gtest/gtest.h>
#include <cstdlib>

#include "ak/alloc/alloc.hpp" // IWYU pragma: keep

class KernelFreeListTest : public ::testing::Test {
protected:
	void* buffer = nullptr;
	AkU64   buffer_size = 1024 * 1024;
	struct ak_alloc_table table{};
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

TEST_F(KernelFreeListTest, WalkBinsAllocateAndFree) {
	AkSize bins = 64;
	AkSize max_size = bins * 32 - 16;
	for (AkU64 size = 16; size <= max_size; size += 32) {
		void* buff = alloc_table_try_malloc(&table, size);
		ASSERT_NE(buff, nullptr) << "size=" << size;
		alloc_table_free(&table, buff, /*side_coalescing*/ 0);
	}
}