#define AK_IMPLEMENTATION
#include "ak.hpp"
#include <cassert>
#include <print> 

using namespace ak;

DefineTask MainTask() noexcept {
	int res = 0;

	std::print("Hello, world! {}\n", res);
	
  	co_return;
}

char buffer[8192];

int main() {
	KernelConfig config = {
		.mem = buffer,
		.memSize = sizeof(buffer),
		.ioEntryCount = 256
	};
	return RunMain(&config, MainTask);
}