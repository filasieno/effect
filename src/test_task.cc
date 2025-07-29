#include "task.hpp"
#include <cassert>

Task a_task() noexcept {
	std::printf("-- A(%p): AAA!\n", &g_kernel.current_task_hdl.promise());
	co_await suspend();
	TaskHdl tt = co_await getCurrentTask();
	std::printf("-- A(%p): Again AAA! Task is: %p\n", &g_kernel.current_task_hdl.promise(), &tt.promise());

}

Task b_task() noexcept {
	std::printf("-- B(%p): BBB!\n", &g_kernel.current_task_hdl.promise());
	co_await suspend();
	std::printf("-- B(%p): Again BBB!\n", &g_kernel.current_task_hdl.promise());
}

Task main_task() noexcept {
	for (int i = 0; i < 10000; ++i) {
		a_task();
		b_task();
	}
	std::printf("-- Task(%p): Hello from Task!\n", &g_kernel.current_task_hdl.promise());
	co_await suspend();
	std::printf("-- Task(%p): Again Hello from Task!\n", &g_kernel.current_task_hdl.promise());

	co_return;
}

int main() {
	return runMainTask(main_task);
}