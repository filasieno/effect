#include "task.hpp"
#include <cassert>

Task main_task() noexcept {
	std::printf("Hello from Task!\n");
	co_await suspend();
	std::printf("Again Hello from Task!\n");
	co_return;
}

int main() {
	return g_kernel.run(main_task);	
}