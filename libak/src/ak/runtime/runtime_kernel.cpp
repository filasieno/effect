#include "ak/runtime/runtime.hpp" // IWYU pragma: keep

alignas(64) ak::Kernel global_kernel_state;

// Global kernel instance declaration
namespace ak {

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

        ak_init_dlink(&global_kernel_state.zombie_list);
        ak_init_dlink(&global_kernel_state.ready_list);
        ak_init_dlink(&global_kernel_state.cthread_list);
        
        return 0;
    }

    AkVoid fini_kernel() noexcept {
        io_uring_queue_exit(&global_kernel_state.io_uring_state);
    }
}



