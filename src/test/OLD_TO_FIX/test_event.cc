#define AK_IMPLEMENTATION
#include "ak.hpp" // IWYU pragma: keep
#include <gtest/gtest.h>

using namespace ak;

static CThread reader_thread(Event* r_ready, Event* w_ready, int *r_signal, int* w_signal, int* value) noexcept {
	int check = 0;
	while (true) {
		EXPECT_LT(check, 12);
		if (*r_signal == 0) {
			co_await ak::wait(r_ready);
			EXPECT_EQ(*r_signal, 1);
			*r_signal = 0;
		} else {
			EXPECT_EQ(*r_signal, 1);
			*r_signal = 0;
		}
		int outValue = *value;
		if (outValue == 0) {
			co_return 0;
		}
		EXPECT_EQ(*w_signal, 0);
		*w_signal = 1;
		int cc = ak::signal(w_ready);
		(void)cc;
		EXPECT_EQ(*w_signal, 1);
		++check;
	}
}

static CThread writer_thread(Event* r_ready, Event* w_ready, int *r_signal, int* w_signal, int* value) noexcept {
	int check = 0;
	int i = 10;
	while (true) {
		EXPECT_LT(check, 12);
		*value = i;
		EXPECT_EQ(*r_signal, 0);
		*r_signal = 1;
		int cc = ak::signal(r_ready);
		(void)cc;
		EXPECT_EQ(*r_signal, 1);
		if (i == 0) {
			co_return 0;
		}
		--i;
		if (*w_signal == 0) {
			co_await ak::wait(w_ready);
			EXPECT_EQ(*w_signal, 1);
			*w_signal = 0;
		} else {
			EXPECT_EQ(*w_signal, 1);
			*w_signal = 0;
		}
		++check;
	}
}

static CThread co_main() noexcept {
	int   value = -1;
	int   r_signal = 0;
	int   w_signal = 0; 
	Event r_ready;
	Event w_ready;

	ak::init(&r_ready);
	ak::init(&w_ready);

	CThread writer = writer_thread(&r_ready, &w_ready, &r_signal, &w_signal, &value);
	CThread reader = reader_thread(&r_ready, &w_ready, &r_signal, &w_signal, &value);
	co_await reader;
	co_await writer;
	std::fflush(stdout);
	co_return 0;
}

class KernelEventTest : public ::testing::Test {
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

TEST_F(KernelEventTest, ReaderWriterHandshake) {
	int rc = run_main(co_main);
	EXPECT_EQ(rc, 0);
}