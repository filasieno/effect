#include "ak.hpp"
#include <cassert>
#include <print> 

/// Condition readyToWrite;
/// Condition readyToRead;
// int value = -1;
// int MAX = 10;

// Task readerTask() noexcept {
// 	while (true) {
// 		co_await readyToRead.wait();
// 		std::print("Read: {}\n", value);
// 		readyToWrite.signal();
// 	    if (value == 0) break;
// 	}
// 	co_return;
// }

// Task writerTask() noexcept {
// 	int i = 10;
// 	while (true) {
// 		co_await readyToWrite.wait();
// 		std::print("Written: {}\n", i); 
// 		readyToRead.signal();
// 		--i;
// 		if (i == 0) break;
// 	}
// }

DefineTask aTask(const char* name) noexcept { 
	std::print("Hello from {}: Step 1\n", name);
	co_await SuspendTask();

	std::print("Hello from {}: Step 2\n", name);
	co_await SuspendTask();

	std::print("Hello from {}: Step 3\n", name);
	co_await SuspendTask();

	std::print("Hello from {}: Step 4\n", name);
	co_await SuspendTask();

	std::print("Hello from {}: Step 5\n", name);
	co_await SuspendTask();

	co_return;
}

DefineTask bTask(const char* name) noexcept { 
	std::print("Hello from {}\n", name);
	co_return;
}

DefineTask MainTask(const char* name) noexcept {
	std::print("Hello from '{}'\n", name);
	TaskHdl a = aTask("A-TASK");
	auto b = bTask("B-TASK");
	co_await a;
	co_await JoinTask(b);
	// readyToWrite.reset(true);
	// readyToRead.reset();
	// Task reader = readerTask();
	// Task writer = writerTask();
	// co_await reader.join();
	// co_await writer.join();
	co_return;
}


int main() {
	KernelConfig config = {
		.mem = nullptr,
		.memSize = 0,
		.ioEntryCount = 256
  	};
	return RunMain(&config, MainTask, "main");
}