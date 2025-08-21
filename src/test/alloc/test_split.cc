#include <gtest/gtest.h>
#include <cstdlib>

#include "ak/alloc/alloc.hpp" // IWYU pragma: keep

using namespace ak;

class KernelAllocSplitTest : public ::testing::Test {
protected:
	Void* buffer = nullptr;
	U64   buffer_size = 1024 * 1024;
	AllocTable table{};
	void SetUp() override {
		buffer = std::malloc(buffer_size);
		ASSERT_NE(buffer, nullptr);
		ASSERT_EQ(priv::init_alloc_table(&table, buffer, buffer_size), 0);
	}
	void TearDown() override {
		std::free(buffer);
		buffer = nullptr;
	}
};

TEST_F(KernelAllocSplitTest, SplitAndReuse) {
	U64 memSize01 = 8096;
	Void* buff01 = ak::priv::try_alloc_table_malloc(&table, memSize01);
	ASSERT_NE(buff01, nullptr);
	ak::priv::alloc_table_free(&table, buff01, 0);

	U64 memSize02 = 16;
	Void* buff02 = ak::priv::try_alloc_table_malloc(&table, memSize02);
	ASSERT_NE(buff02, nullptr);
	ak::priv::alloc_table_free(&table, buff02, 0);
}