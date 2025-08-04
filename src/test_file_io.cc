#include "ak.hpp"
#include <cassert>
#include <print> 

DefineTask MainTask() noexcept {
	int fd = co_await Open("test.txt", O_RDWR | O_CREAT | O_TRUNC | O_NONBLOCK, 0666);
	std::print("fd: {}\n", fd);
	int res = co_await Write(fd, "hello world!\n", 13, 0);
	std::print("written: {}\n", res);
	co_await Close(fd);
	
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