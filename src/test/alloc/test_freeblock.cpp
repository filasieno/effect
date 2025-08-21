#include <gtest/gtest.h>
#include <cstdlib>

#include "ak/alloc/alloc.hpp" // IWYU pragma: keep

using namespace ak;

class KernelAllocFreeBlockTest : public ::testing::Test {
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

TEST_F(KernelAllocFreeBlockTest, SimpleAllocFree) {
	void* buff = ak::priv::try_alloc_table_malloc(&table, 4096);
	ASSERT_NE(buff, nullptr);
	ak::priv::alloc_table_free(&table, buff, 0);
}