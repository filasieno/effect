#define AK_IMPLEMENTATION
#include "ak.hpp" // IWYU pragma: keep
#include <cassert>
#include <print> 

using namespace ak;

DefineTask aTask(const Char* name) noexcept { 
	std::print("Hello from {}: Step 1\n", name);
	co_await SuspendTask();

	std::print("Hello from {}: Step 2\n", name);
	co_await SuspendTask();

	std::print("Hello from {}: Step 3\n", name);
	co_await SuspendTask();

	std::print("Hello from {}: Step 4\n", name);
	co_await SuspendTask();

	std::print("Hello from {}: Step 5\n", name);
	co_await SuspendTask();

	co_return 0;
}

DefineTask bTask(const Char* name) noexcept { 
	std::print("Hello from {}\n", name);
	co_return 0;
}

DefineTask MainTask(const Char* name) noexcept {
	std::print("Hello from '{}'\n", name);
	TaskHdl a = aTask("A-TASK");
	auto b = bTask("B-TASK");
	co_await a;
	co_await JoinTask(b);
	co_return 0;
}

Char buffer[8192];

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