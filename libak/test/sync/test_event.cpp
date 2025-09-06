#include <gtest/gtest.h>

#include "ak.hpp" // IWYU pragma: keep

using namespace ak;

class KernelEventTest : public ::testing::Test {
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


static AkTask reader_thread(AkEvent* r_ready, AkEvent* w_ready, int *r_signal, int* w_signal, int* value) noexcept {
	int check = 0;
	while (true) {
		EXPECT_LT(check, 12);
		if (*r_signal == 0) {
			co_await ak_wait_event(r_ready);
			EXPECT_EQ(*r_signal, 1);
			*r_signal = 0;
		} else {
			EXPECT_EQ(*r_signal, 1);
			*r_signal = 0;
		}
		int outValue = *value;
		std::print("read  : {}\n", outValue);
		if (outValue == 0) {
			co_return 0;
		}
		EXPECT_EQ(*w_signal, 0);
		*w_signal = 1;
		int cc = ak_signal_event(w_ready);
		(void)cc;
		EXPECT_EQ(*w_signal, 1);
		++check;
	}
}

static AkTask writer_thread(AkEvent* r_ready, AkEvent* w_ready, int *r_signal, int* w_signal, int* value) noexcept {
	int check = 0;
	int i = 10;
	while (true) {
		EXPECT_LT(check, 12);
		*value = i;
		std::print("write : {}\n", *value);
		EXPECT_EQ(*r_signal, 0);
		*r_signal = 1;
		int cc = ak_signal_event(r_ready);
		(void)cc;
		EXPECT_EQ(*r_signal, 1);
		if (i == 0) {
			co_return 0;
		}
		--i;
		if (*w_signal == 0) {
			co_await ak_wait_event(w_ready);
			EXPECT_EQ(*w_signal, 1);
			*w_signal = 0;
		} else {
			EXPECT_EQ(*w_signal, 1);
			*w_signal = 0;
		}
		++check;
	}
}

static AkTask co_main() noexcept {
	int   value = -1;
	int   r_signal = 0;
	int   w_signal = 0; 
	AkEvent r_ready;
	AkEvent w_ready;

	ak_init_event(&r_ready);
	ak_init_event(&w_ready);

	AkTask writer = writer_thread(&r_ready, &w_ready, &r_signal, &w_signal, &value);
	AkTask reader = reader_thread(&r_ready, &w_ready, &r_signal, &w_signal, &value);
	co_await reader;
	co_await writer;
	std::fflush(stdout);
	co_return 0;
}



TEST_F(KernelEventTest, ReaderWriterHandshake) {
	int rc = ak_run_main(co_main);
	EXPECT_EQ(rc, 0);
}