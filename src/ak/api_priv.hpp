#pragma once

#include "ak/api.hpp" // IWYU pragma: keep

namespace ak { namespace priv {

#ifndef NDEBUG
    struct KernelDebug {
        // Allocation Table: Set of allocated blocks
        // Promise Table: Set of allocated promises pointing to the `address()` of the Task frame
    };

    inline KernelDebug g_debug_kernel_state;
#endif

    // Allocator routines
    int          init_alloc_table(Void* mem, Size size) noexcept;
    U64          get_alloc_freelist_index(U64 allocation_size) noexcept;
    AllocHeader* next(AllocHeader* h) noexcept;
    AllocHeader* prev(AllocHeader* h) noexcept;
    

    // Scheduling routines
    CThreadCtxHdl   schedule() noexcept;
    CThreadContext* get_linked_context(const utl::DLink* link) noexcept;
    
    // Debug routines
    Void dump_task_count() noexcept;
    Void dump_io_uring_params(const io_uring_params* p);
    Void dump_alloc_table() noexcept;
    Void dump_alloc_block() noexcept;
    
    // Invariant checking routines
    Void check_invariants() noexcept;
    
}} // namespace ak::priv

