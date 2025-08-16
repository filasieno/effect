#define AK_IMPLEMENTATION
#include "ak.hpp" // IWYU pragma: keep
#include <cassert>
#include <print> 

using namespace ak;

DefineTask MainTask() noexcept {
    priv::DebugDumpAllocTable();
    priv::DebugPrintAllocBlocks();
	Void* buff1 = try_alloc_mem(32);
	assert(buff1 != nullptr);
	priv::DebugPrintAllocBlocks();

	Void* buff2 = try_alloc_mem(33);
	assert(buff2 != nullptr);
	priv::DebugPrintAllocBlocks();

	Void* buff3 = try_alloc_mem(63);
	assert(buff3 != nullptr);
	priv::DebugPrintAllocBlocks();

	Void* buff4 = try_alloc_mem(64 - 16);
	assert(buff4 != nullptr);
	priv::DebugPrintAllocBlocks();

	free_mem(buff4);
	priv::DebugPrintAllocBlocks();

	free_mem(buff3);
	priv::DebugPrintAllocBlocks();

	free_mem(buff2);
	priv::DebugPrintAllocBlocks();

	free_mem(buff1);
	priv::DebugPrintAllocBlocks();
	priv::DebugDumpAllocTable();
  	co_return 0;
}

Char buffer[8192];

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