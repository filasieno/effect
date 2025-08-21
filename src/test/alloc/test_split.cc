#include <gtest/gtest.h>
#include "ak/alloc/alloc.hpp" // IWYU pragma: keep

using namespace ak;

class KernelAllocSplitTest : public ::testing::Test {
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

TEST_F(KernelAllocSplitTest, SplitAndReuse) {
	U64 memSize01 = 8096;
	Void* buff01 = try_alloc_mem(memSize01);
	ASSERT_NE(buff01, nullptr);
	free_mem(buff01);

	U64 memSize02 = 16;
	Void* buff02 = try_alloc_mem(memSize02);
	ASSERT_NE(buff02, nullptr);
	free_mem(buff02);
}