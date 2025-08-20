#define AK_IMPLEMENTATION
#include "ak.hpp" // IWYU pragma: keep
#include <cassert>
#include <print> 

using namespace ak;

CThread a_thread(const Char* name) noexcept { 
	std::print("Hello from {}: Step 1\n", name);
	co_await suspend();

	std::print("Hello from {}: Step 2\n", name);
	co_await suspend();

	std::print("Hello from {}: Step 3\n", name);
	co_await suspend();

	std::print("Hello from {}: Step 4\n", name);
	co_await suspend();

	std::print("Hello from {}: Step 5\n", name);
	co_await suspend();

	co_return 0;
}

CThread b_thread(const Char* name) noexcept { 
	std::print("Hello from {}\n", name);
	co_return 0;
}

CThread co_main(const Char* name) noexcept {
	std::print("Hello from '{}'\n", name);
	auto a = a_thread("A-TASK");
	auto b = b_thread("B-TASK");
	co_await a;
	co_await b;
	co_return 0;
}

Char buffer[8192];

int main() {
	KernelConfig config = {
		.mem = buffer,
		.memSize = sizeof(buffer),
		.ioEntryCount = 256
  	};
	int res = init_kernel(&config);
	assert(res == 0);
	if (run_main(co_main, "main") != 0) {
		std::print("main failed\n");
		std::abort();
		// Unreachable
	}
	fini_kernel();
	return 0;
}