#define AK_IMPLEMENTATION
#include "ak.hpp" // IWYU pragma: keep
#include <cassert>
#include <print> 

using namespace ak;

CThread co_main() noexcept {
    priv::DebugPrintAllocBlocks();
	priv::DebugDumpAllocTable();
    	
	U64 memSize01 = 8096;
	Void* buff01 = try_alloc_mem(memSize01);
	assert(buff01 != nullptr);
	
	priv::DebugPrintAllocBlocks();
	priv::DebugDumpAllocTable();

	free_mem(buff01);

	priv::DebugPrintAllocBlocks();
	priv::DebugDumpAllocTable();

	U64 memSize02 = 16;
	Void* buff02 = try_alloc_mem(memSize02);
	assert(buff02 != nullptr);

	priv::DebugPrintAllocBlocks();
	priv::DebugDumpAllocTable();

	free_mem(buff02);

	priv::DebugPrintAllocBlocks();
	priv::DebugDumpAllocTable();
	
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
	if (run_main(&config, co_main) != 0) {
		std::print("main failed\n");
		std::abort();
		// Unreachable
	}
	free(buffer);
	return 0;
}