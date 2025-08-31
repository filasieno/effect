#pragma once

#include "ak/alloc/alloc.hpp"         // IWYU pragma: keep
#include "ak/runtime/runtime_api.hpp" // IWYU pragma: keep

namespace ak::priv {
    
    // Scheduler task routines
    CThread::Hdl schedule_next_thread() noexcept;

    // Debug routines
    AkVoid check_invariants() noexcept;
    AkVoid dump_task_count() noexcept;
    AkVoid dump_io_uring_params(const io_uring_params* p);
    AkVoid dump_alloc_table() noexcept;
    AkVoid dump_alloc_block() noexcept;

           
}


