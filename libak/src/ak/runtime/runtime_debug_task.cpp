  #include "ak/runtime/runtime.hpp" // IWYU pragma: keep


inline static void do_check_task_count_invariant() noexcept;


void check_task_count_invariant() noexcept 
{
    do_check_task_count_invariant();

}

void runtime_check_invariants() noexcept 
{
    if constexpr (AK_IS_DEBUG_MODE) {
        // check the Task invariants
        do_check_task_count_invariant();

        // Ensure that the current Task is valid
        // if (gKernel.currentTask.isValid()) std::abort();
    }
}

void runtime_dump_task_count() noexcept {
    if constexpr (AK_TRACE_DEBUG_CODE) {
        int running_count = global_kernel_state.current_task != AkCoroutineHandle() ? 1 : 0;
        std::print("- {} Running\n", running_count);
        std::print("  {} Ready\n", global_kernel_state.ready_task_count);
        std::print("  {} Waiting\n", global_kernel_state.waiting_task_count);
        std::print("  {} IO waiting\n", global_kernel_state.iowaiting_task_count);
        std::print("  {} Zombie\n", global_kernel_state.zombie_task_count);
    }
}

inline static void do_check_task_count_invariant() noexcept 
{
    if constexpr (AK_IS_DEBUG_MODE) {
        int running_count = global_kernel_state.current_task != AkCoroutineHandle() ? 1 : 0;
        bool condition = global_kernel_state.task_count == running_count + global_kernel_state.ready_task_count + global_kernel_state.waiting_task_count + global_kernel_state.iowaiting_task_count + global_kernel_state.zombie_task_count;
        if (!condition) {
            runtime_dump_task_count();
            std::abort();
        }
    }
}

