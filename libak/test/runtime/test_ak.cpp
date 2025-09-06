#include <gtest/gtest.h>

#include "ak.hpp" // IWYU pragma: keep

using namespace ak;


class KernelAkTest : public ::testing::Test {
protected:
	void* buffer = nullptr;
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

static AkTask a_thread() noexcept {
	co_await ak_suspend_task();
	co_await ak_suspend_task();
	co_await ak_suspend_task();
	co_await ak_suspend_task();
	co_await ak_suspend_task();
	co_return 0;
}

static AkTask b_thread() noexcept {
	co_return 0;
}

static AkTask co_main() noexcept {
	auto a = a_thread();
	auto b = b_thread();
	co_await a;
	co_await b;
	co_return 0;
}

TEST_F(KernelAkTest, CoroutineRun) {
	int rc = ak_run_main(co_main);
	EXPECT_EQ(rc, 0);
}