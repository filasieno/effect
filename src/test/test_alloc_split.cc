#define AK_IMPLEMENTATION
#include "ak.hpp" // IWYU pragma: keep
#include <cassert>
#include <print> 

using namespace ak;

DefineTask MainTask() noexcept {
    priv::DebugPrintAllocBlocks();
	priv::DebugDumpAllocTable();
    	
	U64 memSize01 = 8096;
	void* buff01 = TryAllocMem(memSize01);
	assert(buff01 != nullptr);
	
	priv::DebugPrintAllocBlocks();
	priv::DebugDumpAllocTable();

	FreeMem(buff01);

	priv::DebugPrintAllocBlocks();
	priv::DebugDumpAllocTable();

	U64 memSize02 = 16;
	void* buff02 = TryAllocMem(memSize02);
	assert(buff02 != nullptr);

	priv::DebugPrintAllocBlocks();
	priv::DebugDumpAllocTable();

	FreeMem(buff02);

	priv::DebugPrintAllocBlocks();
	priv::DebugDumpAllocTable();
	
  	co_return 0;
}



int main() {
	U64 bufferSize = 1024 * 1024;
	void* buffer = malloc(bufferSize);
	KernelConfig config = {
		.mem          = buffer,
		.memSize      = bufferSize,
		.ioEntryCount = 256
	};
	if (RunMain(&config, MainTask) != 0) {
		std::print("main failed\n");
		std::abort();
		// Unreachable
	}
	free(buffer);
	return 0;
}