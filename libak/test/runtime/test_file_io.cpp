#include <gtest/gtest.h>

#include "ak.hpp" // IWYU pragma: keep

using namespace ak;

class KernelFileIOTest : public ::testing::Test {
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

static AkTask io_sequence(const char* p) noexcept {
	int fd = co_await ak_os_io_open(p, O_RDWR | O_CREAT | O_TRUNC | O_NONBLOCK, 0666);
	std::print("open fd: {}\n", fd);
	EXPECT_GE(fd, 0);
	int wr = co_await ak_os_io_write(fd, "hello world!\n", 13, 0);
	std::print("written : {}\n", wr);
	EXPECT_GE(wr, 0);
	int cl = co_await ak_os_io_close(fd);
	std::print("close res: {}\n", cl);
	EXPECT_GE(cl, 0);
	int ul = co_await ak_os_io_unlink(p, 0);
	std::print("unlink res: {}\n", ul);
	EXPECT_GE(ul, 0);
	co_return 0;
}

TEST_F(KernelFileIOTest, BasicOpenWriteCloseUnlink) {
	const char* path = "test_file_io.txt";
	int res = ak_run_main(io_sequence, path);
	EXPECT_EQ(res, 0);
}