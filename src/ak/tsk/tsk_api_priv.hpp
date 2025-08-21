#pragma once

#include "ak/api_priv.hpp" // IWYU pragma: keep

namespace ak {
    namespace priv 
    {
        struct RunSchedulerOp;
        struct TerminateSchedulerOp;
                
        // Boot routines
        template <typename... Args>
        BootCThread boot_main_proc(CThread(*main_proc)(Args ...) noexcept, Args ... args) noexcept;

        template <typename... Args>
        CThread scheduler_main_proc(CThread(*main_proc)(Args ...) noexcept, Args... args) noexcept;
        
        // Scheduler task routines
        constexpr RunSchedulerOp       run_scheduler() noexcept;
        constexpr TerminateSchedulerOp terminate_scheduler() noexcept;
        constexpr Void                 destroy_scheduler(CThread hdl) noexcept;
    
    }       
}
