#define AK_IMPLEMENTATION
#include "ak.hpp" // IWYU pragma: keep
#include <cassert>
#include <print> 

using namespace ak;

CThread co_main() noexcept {
    priv::dump_alloc_block();
	priv::dump_alloc_table();
    	
	U64 memSize01 = 8096;
	Void* buff01 = try_alloc_mem(memSize01);
	assert(buff01 != nullptr);
	
	priv::dump_alloc_block();
	priv::dump_alloc_table();

	free_mem(buff01);

	priv::dump_alloc_block();
	priv::dump_alloc_table();

	U64 memSize02 = 16;
	Void* buff02 = try_alloc_mem(memSize02);
	assert(buff02 != nullptr);

	priv::dump_alloc_block();
	priv::dump_alloc_table();

	free_mem(buff02);

	priv::dump_alloc_block();
	priv::dump_alloc_table();
	
  	co_return 0;
}



int main() {
	U64 bufferSize = 1024 * 1024;
	Void* buffer = malloc(bufferSize);
	KernelConfig config = {
		.mem          = buffer,
		.memSize      = bufferSize,
		.ioEntryCount = 256
	};
	int init_rc = init_kernel(&config);
	assert(init_rc == 0);
	if (run_main(co_main) != 0) {
		std::print("main failed\n");
		std::abort();
		// Unreachable
	}
	fini_kernel();
	free(buffer);
	return 0;
}