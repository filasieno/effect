#define AK_IMPLEMENTATION
#include "ak.hpp"
#include <cassert>
#include <print> 

using namespace ak;

DefineTask MainTask() noexcept {
    priv::DebugDumpAllocTable(&gKernel.allocTable);
    priv::DebugPrintAllocBlocks(&gKernel.allocTable);
  	co_return;
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
	return 0;
}