#define AK_IMPLEMENTATION
#include "ak.hpp" // IWYU pragma: keep
#include <cassert>
#include <print> 

using namespace ak;

CThread co_main(const Char* name) noexcept {
	int res;

	const Char* path = "test.txt";
    int fd = co_await io_open(path, O_RDWR | O_CREAT | O_TRUNC | O_NONBLOCK, 0666);
	std::print("Open res: {}\n", fd);
    res = co_await io_write(fd, "hello world!\n", 13, 0);
	std::print("written: {}\n", res);
    res = co_await io_close(fd);
	std::print("Close res: {}\n", res);
    res = co_await io_unlink(path, 0);
	std::print("Unlink res: {}\n", res);
	
  	co_return 0;
}

Char buffer[8192];

int main() {
	KernelConfig config = {
		.mem = buffer,
		.memSize = sizeof(buffer),
		.ioEntryCount = 256
  	};
	
	if (run_main_cthread(&config, co_main, "main") != 0) {
		std::print("main failed\n");
		std::abort();
		// Unreachable
	}
	return 0;
}