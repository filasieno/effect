#define AK_IMPLEMENTATION
#include "ak.hpp" // IWYU pragma: keep
#include <cassert>
#include <print> 

using namespace ak;

DefineTask MainTask() noexcept {
    priv::DebugDumpAllocTable();
    priv::DebugPrintAllocBlocks();
	Size bins = 253;
	Size maxSize = bins * 32  - 16;
	for (U64 memSize = 16; memSize <= maxSize; memSize += 32) {
		Void* buff = try_alloc_mem(memSize);
		assert(buff != nullptr);
		free_mem(buff);
	}
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
	if (RunMain(&config, MainTask) != 0) {
		std::print("main failed\n");
		std::abort();
		// Unreachable
	}
	priv::DebugPrintAllocBlocks();
	priv::DebugDumpAllocTable();
	free(buffer);
	return 0;
}