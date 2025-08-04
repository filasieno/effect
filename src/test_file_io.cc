#include "ak.hpp"
#include <cassert>
#include <print> 

DefineTask MainTask() noexcept {
	int res;

	const char* path = "test.txt";
	int fd = co_await XOpen(path, O_RDWR | O_CREAT | O_TRUNC | O_NONBLOCK, 0666);
	std::print("Open res: {}\n", fd);
	res = co_await XWrite(fd, "hello world!\n", 13, 0);
	std::print("written: {}\n", res);
	res = co_await XClose(fd);
	std::print("Close res: {}\n", res);
	res = co_await XUnlink(path, 0);
	std::print("Unlink res: {}\n", res);
	
  	co_return;
}

int main() {
	KernelConfig config = {
		.mem = nullptr,
		.memSize = 0,
		.ioEntryCount = 256
	};
	return RunMain(&config, MainTask);
}