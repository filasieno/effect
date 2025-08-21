#pragma once

#include "ak/alloc/alloc.hpp"         // IWYU pragma: keep
#include "ak/runtime/runtime_api.hpp" // IWYU pragma: keep

namespace ak {
    namespace priv 
    {
                
        // Boot routines
        template <typename... Args>
        BootCThread boot_main_proc(CThread(*main_proc)(Args ...) noexcept, Args ... args) noexcept;

        template <typename... Args>
        CThread scheduler_main_proc(CThread(*main_proc)(Args ...) noexcept, Args... args) noexcept;
        


        // Scheduler task routines
        CThread::Hdl      schedule_cthread() noexcept;
        CThread::Context* get_linked_cthread_context(const DLink* link) noexcept;

        // Debug routines
        Void check_invariants() noexcept;
        Void dump_task_count() noexcept;
        // Void dump_io_uring_params(const io_uring_params* p);
        // Void dump_alloc_table() noexcept;
        // Void dump_alloc_block() noexcept;
    
    }       
}


