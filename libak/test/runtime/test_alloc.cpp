#include <gtest/gtest.h>

#include "ak.hpp" // IWYU pragma: keep

using namespace ak;

class KernelAllocTest : public ::testing::Test {
protected:
	Void* buffer = nullptr;
	U64   buffer_size = 8192;
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

TEST_F(KernelAllocTest, BasicAllocFree) {
	Void* buff1 = try_alloc_mem(32);
	ASSERT_NE(buff1, nullptr);
	Void* buff2 = try_alloc_mem(33);
	ASSERT_NE(buff2, nullptr);
	Void* buff3 = try_alloc_mem(63);
	ASSERT_NE(buff3, nullptr);
	Void* buff4 = try_alloc_mem(64 - 16);
	ASSERT_NE(buff4, nullptr);
	free_mem(buff4);
	free_mem(buff3);
	free_mem(buff2);
	free_mem(buff1);
}