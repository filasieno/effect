#define AK_IMPLEMENTATION
#include "ak.hpp" // IWYU pragma: keep
#include <gtest/gtest.h>

using namespace ak;

static CThread a_thread() noexcept {
	co_await suspend();
	co_await suspend();
	co_await suspend();
	co_await suspend();
	co_await suspend();
	co_return 0;
}

static CThread b_thread() noexcept {
	co_return 0;
}

static CThread co_main() noexcept {
	auto a = a_thread();
	auto b = b_thread();
	co_await a;
	co_await b;
	co_return 0;
}

class KernelAkTest : public ::testing::Test {
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

TEST_F(KernelAkTest, CoroutineRun) {
	int rc = run_main(co_main);
	EXPECT_EQ(rc, 0);
}