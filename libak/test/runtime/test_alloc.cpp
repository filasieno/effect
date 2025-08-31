#include <gtest/gtest.h>

#include "ak.hpp" // IWYU pragma: keep

using namespace ak;

class KernelAllocTest : public ::testing::Test {
protected:
	AkVoid* buffer = nullptr;
	AkU64   buffer_size = 8192;
	void SetUp() override {
		buffer = std::malloc(buffer_size);
		ASSERT_NE(buffer, nullptr);
		AkKernelConfig config{ .mem_buffer = buffer, .mem_buffer_size = buffer_size, .io_uring_entry_count = 256 };
		ASSERT_EQ(ak_init_kernel(&config), 0);
	}
	void TearDown() override {
		ak_fini_kernel();
		std::free(buffer);
		buffer = nullptr;
	}
};

TEST_F(KernelAllocTest, BasicAllocFree) {
	AkVoid* buff1 = ak_malloc(32);
	ASSERT_NE(buff1, nullptr);
	AkVoid* buff2 = ak_malloc(33);
	ASSERT_NE(buff2, nullptr);
	AkVoid* buff3 = ak_malloc(63);
	ASSERT_NE(buff3, nullptr);
	AkVoid* buff4 = ak_malloc(64 - 16);
	ASSERT_NE(buff4, nullptr);
	ak_free(buff4);
	ak_free(buff3);
	ak_free(buff2);
	ak_free(buff1);
}