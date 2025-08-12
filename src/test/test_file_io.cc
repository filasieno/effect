#define AK_IMPLEMENTATION
#include "ak.hpp"
#include <cassert>
#include <print> 

using namespace ak;

DefineTask MainTask(const char* name) noexcept {
	int res;

	const char* path = "test.txt";
    int fd = co_await IOOpen(path, O_RDWR | O_CREAT | O_TRUNC | O_NONBLOCK, 0666);
	std::print("Open res: {}\n", fd);
    res = co_await IOWrite(fd, "hello world!\n", 13, 0);
	std::print("written: {}\n", res);
    res = co_await IOClose(fd);
	std::print("Close res: {}\n", res);
    res = co_await IOUnlink(path, 0);
	std::print("Unlink res: {}\n", res);
	
  	co_return;
}

char buffer[8192];

int main() {
	KernelConfig config = {
		.mem = buffer,
		.memSize = sizeof(buffer),
		.ioEntryCount = 256
  	};
	
	if (RunMain(&config, MainTask, "main") != 0) {
		std::print("main failed\n");
		std::abort();
		// Unreachable
	}
	return 0;
}