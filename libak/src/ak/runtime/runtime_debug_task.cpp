  #include "ak/runtime/runtime.hpp" // IWYU pragma: keep

  namespace ak::priv {   

    // Task runtime debug utilities
    // ----------------------------------------------------------------------------------------------------------------

    Void dump_task_count() noexcept {
        if constexpr (priv::TRACE_DEBUG_CODE) {
            int running_count = global_kernel_state.current_cthread != CThread::Hdl() ? 1 : 0;
            std::print("- {} Running\n", running_count);
            std::print("  {} Ready\n", global_kernel_state.ready_cthread_count);
            std::print("  {} Waiting\n", global_kernel_state.waiting_cthread_count);
            std::print("  {} IO waiting\n", global_kernel_state.iowaiting_cthread_count);
            std::print("  {} Zombie\n", global_kernel_state.zombie_cthread_count);
        }
    }

    // Check Task Invariants
    // ----------------------------------------------------------------------------------------------------------------

    static Void do_check_task_count_invariant() noexcept {
        int running_count = global_kernel_state.current_cthread != CThread::Hdl() ? 1 : 0;
        Bool condition = global_kernel_state.cthread_count == running_count + global_kernel_state.ready_cthread_count + global_kernel_state.waiting_cthread_count + global_kernel_state.iowaiting_cthread_count + global_kernel_state.zombie_cthread_count;
        if (!condition) {
            dump_task_count();
            abort();
        }
    }

    Void check_task_count_invariant() noexcept {
        if constexpr (IS_DEBUG_MODE) {
            do_check_task_count_invariant();
        }
    }

    Void check_invariants() noexcept {
        if constexpr (IS_DEBUG_MODE) {
            // check the Task invariants
            do_check_task_count_invariant();

            // Ensure that the current Task is valid
            // if (gKernel.currentTask.isValid()) std::abort();
        }
    }
}