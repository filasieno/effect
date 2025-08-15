#define AK_IMPLEMENTATION
#include "ak.hpp"
#include <cassert>
#include <print> 

using namespace ak;

DefineTask MainTask() noexcept {
    priv::DebugDumpAllocTable();
    priv::DebugPrintAllocBlocks();
	void* buff1 = TryAllocMem(32);
	assert(buff1 != nullptr);
	priv::DebugPrintAllocBlocks();

	void* buff2 = TryAllocMem(33);
	assert(buff2 != nullptr);
	priv::DebugPrintAllocBlocks();

	void* buff3 = TryAllocMem(63);
	assert(buff3 != nullptr);
	priv::DebugPrintAllocBlocks();

	void* buff4 = TryAllocMem(64 - 16);
	assert(buff4 != nullptr);
	priv::DebugPrintAllocBlocks();

	FreeMem(buff4);
	priv::DebugPrintAllocBlocks();

	FreeMem(buff3);
	priv::DebugPrintAllocBlocks();

	FreeMem(buff2);
	priv::DebugPrintAllocBlocks();

	FreeMem(buff1);
	priv::DebugPrintAllocBlocks();
	priv::DebugDumpAllocTable();
  	co_return 0;
}

char buffer[8192];

int main() {
	KernelConfig config = {
		.mem          = buffer,
		.memSize      = sizeof(buffer),
		.ioEntryCount = 256
	};
	if (RunMain(&config, MainTask) != 0) {
		std::print("main failed\n");
		std::abort();
		// Unreachable
	}
	priv::DebugPrintAllocBlocks();
	priv::DebugDumpAllocTable();
	return 0;
}