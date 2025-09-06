#include <gtest/gtest.h>
#include <cstdlib>

#include "ak/alloc/alloc.hpp" // IWYU pragma: keep

class KernelAllocSplitTest : public ::testing::Test {
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

TEST_F(KernelAllocSplitTest, SplitAndReuse) {
	AkU64 memSize01 = 8096;
	void* buff01 = alloc_table_try_malloc(&table, memSize01);
	ASSERT_NE(buff01, nullptr);
	alloc_table_free(&table, buff01, 0);

	AkU64 memSize02 = 16;
	void* buff02 = alloc_table_try_malloc(&table, memSize02);
	ASSERT_NE(buff02, nullptr);
	alloc_table_free(&table, buff02, 0);
}