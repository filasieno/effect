#include <gtest/gtest.h>
#include <cstdlib>

#include "ak/alloc/alloc.hpp" // IWYU pragma: keep


class KernelAllocFreeBlockTest : public ::testing::Test {
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

TEST_F(KernelAllocFreeBlockTest, SimpleAllocFree) {
	void* buff = alloc_table_try_malloc(&table, 4096);
	ASSERT_NE(buff, nullptr);
	alloc_table_free(&table, buff, 0);
}