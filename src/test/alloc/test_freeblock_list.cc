#include <gtest/gtest.h>
#include <cstdlib>

#include "ak/alloc/alloc.hpp" // IWYU pragma: keep

using namespace ak;

class KernelFreeListTest : public ::testing::Test {
protected:
	Void* buffer = nullptr;
	U64   buffer_size = 1024 * 1024;
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

TEST_F(KernelFreeListTest, WalkBinsAllocateAndFree) {
	Size bins = 64;
	Size max_size = bins * 32 - 16;
	for (U64 size = 16; size <= max_size; size += 32) {
		Void* buff = ak::priv::try_alloc_table_malloc(&table, size);
		ASSERT_NE(buff, nullptr) << "size=" << size;
		ak::priv::alloc_table_free(&table, buff, /*side_coalescing*/ 0);
	}
}