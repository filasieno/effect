#define AK_IMPLEMENTATION
#include "ak.hpp" // IWYU pragma: keep
#include <gtest/gtest.h>

using namespace ak;

class KernelFileIOTest : public ::testing::Test {
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

static CThread io_sequence(const Char* p) noexcept {
	int fd = co_await io_open(p, O_RDWR | O_CREAT | O_TRUNC | O_NONBLOCK, 0666);
	EXPECT_GE(fd, 0);
	int wr = co_await io_write(fd, "hello world!\n", 13, 0);
	EXPECT_GE(wr, 0);
	int cl = co_await io_close(fd);
	EXPECT_GE(cl, 0);
	int ul = co_await io_unlink(p, 0);
	EXPECT_GE(ul, 0);
	co_return 0;
}

TEST_F(KernelFileIOTest, BasicOpenWriteCloseUnlink) {
	const Char* path = "test_file_io.txt";
	int res = run_main(io_sequence, path);
	EXPECT_EQ(res, 0);
}