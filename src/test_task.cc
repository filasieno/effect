#include "task.hpp"
#include <cassert>
#include <print> 

Condition readyToWrite;
Condition readyToRead;
int value = -1;
int MAX = 10;

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

Task aTask() noexcept { 
	std::print("Hello from A: Step 1\n");
	co_await suspend();

	std::print("Hello from A: Step 2\n");
	co_await suspend();

	std::print("Hello from A: Step 3\n");
	co_await suspend();

	std::print("Hello from A: Step 4\n");
	co_await suspend();

	std::print("Hello from A: Step 5\n");
	co_await suspend();

	co_return;
}

Task aChild() noexcept { 
	std::print("Hello from A Child: Step 1\n");
	co_await suspend();

	std::print("Hello from A Child: Step 2\n");
	co_await suspend();

	std::print("Hello from A Child: Step 3\n");
	co_await suspend();

	std::print("Hello from A Child: Step 4\n");
	co_await suspend();

	std::print("Hello from A Child: Step 5\n");
	co_await suspend();

	co_return;
}

Task bTask() noexcept { 
	std::print("Hello from B\n");
	co_return;
}


Task main_task() noexcept {
	Task a = aTask();
	Task b = bTask();
	co_await a;
	co_await b;
	// readyToWrite.reset(true);
	// readyToRead.reset();
	// Task reader = readerTask();
	// Task writer = writerTask();
	// co_await reader.join();
	// co_await writer.join();
	co_return;
}


int main() {
	int res = runMainTask(main_task);
	std::print("main_task returned {}\n", res);
	return res;
}