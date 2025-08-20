#define AK_IMPLEMENTATION
#include "ak.hpp" // IWYU pragma: keep
#include <cassert>
#include <print> 

using namespace ak;

CThread co_main() noexcept {
    priv::dump_alloc_table();
    priv::dump_alloc_block();
	void* buff = ak::try_alloc_mem(4096);
    ak::free_mem(buff);
  	co_return 0;
}

int main() {
	U64 buffer_size = 1024 * 1024;
	Void* buffer = malloc(buffer_size);
	KernelConfig config = {
		.mem          = buffer,
		.memSize      = buffer_size,
		.ioEntryCount = 256
	};
	if (run_main_cthread(&config, co_main) != 0) {
		std::print("main failed\n");
		std::abort();
		// Unreachable
	}
	priv::dump_alloc_block();
	priv::dump_alloc_table();
	free(buffer);
	return 0;
}