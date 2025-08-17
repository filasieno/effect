#define AK_IMPLEMENTATION
#include "ak.hpp" // IWYU pragma: keep
#include <cassert>
#include <print> 

using namespace ak;

CThread co_main() noexcept {
    priv::dump_alloc_table();
    priv::dump_alloc_block();
	Void* buff1 = try_alloc_mem(32);
	assert(buff1 != nullptr);
	priv::dump_alloc_block();

	Void* buff2 = try_alloc_mem(33);
	assert(buff2 != nullptr);
	priv::dump_alloc_block();

	Void* buff3 = try_alloc_mem(63);
	assert(buff3 != nullptr);
	priv::dump_alloc_block();

	Void* buff4 = try_alloc_mem(64 - 16);
	assert(buff4 != nullptr);
	priv::dump_alloc_block();

	free_mem(buff4);
	priv::dump_alloc_block();

	free_mem(buff3);
	priv::dump_alloc_block();

	free_mem(buff2);
	priv::dump_alloc_block();

	free_mem(buff1);
	priv::dump_alloc_block();
	priv::dump_alloc_table();
  	co_return 0;
}

Char buffer[8192];

int main() {
	KernelConfig config = {
		.mem          = buffer,
		.memSize      = sizeof(buffer),
		.ioEntryCount = 256
	};
	if (run_main_loop(&config, co_main) != 0) {
		std::print("main failed\n");
		std::abort();
		// Unreachable
	}
	priv::dump_alloc_block();
	priv::dump_alloc_table();
	return 0;
}