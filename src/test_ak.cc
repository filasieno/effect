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

DefineTask aTask() noexcept { 
	std::print("Hello from A: Step 1\n");
	co_await SuspendTask();

	std::print("Hello from A: Step 2\n");
	co_await SuspendTask();

	std::print("Hello from A: Step 3\n");
	co_await SuspendTask();

	std::print("Hello from A: Step 4\n");
	co_await SuspendTask();

	std::print("Hello from A: Step 5\n");
	co_await SuspendTask();

	co_return;
}

DefineTask aChild() noexcept { 
	std::print("Hello from A Child: Step 1\n");
	co_await SuspendTask();

	std::print("Hello from A Child: Step 2\n");
	co_await SuspendTask();

	std::print("Hello from A Child: Step 3\n");
	co_await SuspendTask();

	std::print("Hello from A Child: Step 4\n");
	co_await SuspendTask();

	std::print("Hello from A Child: Step 5\n");
	co_await SuspendTask();

	co_return;
}

DefineTask bTask() noexcept { 
	std::print("Hello from B\n");
	co_return;
}



DefineTask main_task() noexcept {
	TaskHdl a = aTask();
	auto b = bTask();
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
	int res = RunMain(main_task);
	std::print("main_task returned {}\n", res);
	return res;
}