#include "task.hpp"
#include <cassert>

Task a_task() noexcept {
	std::printf("-- A(%p): AAA!\n", &g_kernel.current_task_hdl.promise());
	co_await suspend();
	std::printf("-- A(%p): Again AAA!\n", &g_kernel.current_task_hdl.promise());

}

Task b_task() noexcept {
	std::printf("-- B(%p): BBB!\n", &g_kernel.current_task_hdl.promise());
	co_await suspend();
	std::printf("-- B(%p): Again BBB!\n", &g_kernel.current_task_hdl.promise());
}

Task main_task() noexcept {
	Task a = a_task();
	Task b = b_task();
	std::printf("-- Task(%p): Hello from Task!\n", &g_kernel.current_task_hdl.promise());
	co_await suspend();
	std::printf("-- Task(%p): Again Hello from Task!\n", &g_kernel.current_task_hdl.promise());

	co_return;
}

int main() {
	return runMainTask(main_task);
}