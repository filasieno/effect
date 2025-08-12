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

int main() {
	KernelConfig config = {
		.mem = nullptr,
		.memSize = 0,
		.ioEntryCount = 256
	};
	return RunMain(&config, MainTask);
}