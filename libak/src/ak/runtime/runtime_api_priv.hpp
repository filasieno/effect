#pragma once

#include "ak/alloc/alloc.hpp"         // IWYU pragma: keep
#include "ak/runtime/runtime_api.hpp" // IWYU pragma: keep

namespace ak::priv {
    
    // Scheduler task routines
    CThread::Hdl schedule_next_thread() noexcept;

    // Debug routines
    Void check_invariants() noexcept;
    Void dump_task_count() noexcept;
    Void dump_io_uring_params(const io_uring_params* p);
    Void dump_alloc_table() noexcept;
    Void dump_alloc_block() noexcept;

           
}


