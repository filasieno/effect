#include "task.hpp"
#include <cassert>

Task main_task() noexcept {
	std::printf("-- Task(%p): Hello from Task!\n", &g_kernel.current_task_hdl.promise());
	co_await suspend();
	std::printf("-- Task(%p): Again Hello from Task!\n", &g_kernel.current_task_hdl.promise());
	co_return;
}

int main() {
	return startMainTask(main_task);
}