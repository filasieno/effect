#include <gtest/gtest.h>
#include "ak/alloc/alloc.hpp" // IWYU pragma: keep

using namespace ak;

class KernelAllocFreeBlockTest : public ::testing::Test {
protected:
	Void* buffer = nullptr;
	U64   buffer_size = 1024 * 1024;
	void SetUp() override {
		buffer = std::malloc(buffer_size);
		ASSERT_NE(buffer, nullptr);
		KernelConfig config{ .mem = buffer, .memSize = buffer_size, .ioEntryCount = 256 };
		ASSERT_EQ(init_kernel(&config), 0);
	}
	void TearDown() override {
		fini_kernel();
		std::free(buffer);
		buffer = nullptr;
	}
};

TEST_F(KernelAllocFreeBlockTest, SimpleAllocFree) {
	void* buff = ak::try_alloc_mem(4096);
	ASSERT_NE(buff, nullptr);
	ak::free_mem(buff);
}