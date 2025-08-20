#define AK_IMPLEMENTATION
#include "ak.hpp" // IWYU pragma: keep
#include <gtest/gtest.h>

using namespace ak;
using namespace ak::priv;

class KernelFreeListTest : public ::testing::Test {
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

TEST_F(KernelFreeListTest, WalkBinsAllocateAndFree) {
	Size bins = 64;
	Size max_size = bins * 32 - 16;
	for (U64 size = 16; size <= max_size; size += 32) {
		Void* buff = try_alloc_mem(size);
		ASSERT_NE(buff, nullptr) << "size=" << size;
		free_mem(buff);
	}
}