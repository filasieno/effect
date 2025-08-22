#include "ak/runtime/runtime.hpp" // IWYU pragma: keep

// Global kernel instance declaration
namespace ak {
    alignas(64) Kernel global_kernel_state;

    // Kernel init/fini
    // ----------------------------------------------------------------------------------------------------------------

    int init_kernel(KernelConfig* config) noexcept {
        using namespace priv;
        
        if (init_alloc_table(&global_kernel_state.alloc_table, config->mem, config->memSize) != 0) {
            return -1;
        }

        int res = io_uring_queue_init(config->ioEntryCount, &global_kernel_state.io_uring_state, 0);
        if (res < 0) {
            std::print("io_uring_queue_init failed\n");
            return -1;
        }

        global_kernel_state.mem = config->mem;
        global_kernel_state.mem_size = config->memSize;
        global_kernel_state.cthread_count = 0;
        global_kernel_state.ready_cthread_count = 0;
        global_kernel_state.waiting_cthread_count = 0;
        global_kernel_state.iowaiting_cthread_count = 0;
        global_kernel_state.zombie_cthread_count = 0;
        global_kernel_state.interrupted = 0;

        global_kernel_state.current_cthread.reset();
        global_kernel_state.scheduler_cthread.reset();

        init_dlink(&global_kernel_state.zombie_list);
        init_dlink(&global_kernel_state.ready_list);
        init_dlink(&global_kernel_state.cthread_list);
        
        return 0;
    }

    Void fini_kernel() noexcept {
        io_uring_queue_exit(&global_kernel_state.io_uring_state);
    }
}



