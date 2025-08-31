#include <gtest/gtest.h>
#include <cstdlib>

#include "ak/alloc/alloc.hpp" // IWYU pragma: keep

using namespace ak;

class KernelAllocFreeBlockTest : public ::testing::Test {
protected:
	AkVoid* buffer = nullptr;
	AkU64   buffer_size = 1024 * 1024;
	AkAllocTable table{};
	void SetUp() override {
		buffer = std::malloc(buffer_size);
		ASSERT_NE(buffer, nullptr);
		ASSERT_EQ(priv::alloc_table_init(&table, buffer, buffer_size), 0);
	}
	void TearDown() override {
		std::free(buffer);
		buffer = nullptr;
	}
};

TEST_F(KernelAllocFreeBlockTest, SimpleAllocFree) {
	void* buff = ak::priv::alloc_table_try_malloc(&table, 4096);
	ASSERT_NE(buff, nullptr);
	ak::priv::alloc_table_free(&table, buff, 0);
}