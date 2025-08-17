#pragma once

#include "ak/api.hpp" // IWYU pragma: keep

namespace ak { namespace priv {

#ifndef NDEBUG
    struct KernelDebug {
        // Allocation Table: Set of allocated blocks
        // Promise Table: Set of allocated promises pointing to the `address()` of the Task frame
    };

    inline KernelDebug gKernelDebug;
#endif

    // Allocator routines
    int          InitAllocTable(Void* mem, Size size) noexcept;
    AllocHeader* NextAllocHeaderPtr(AllocHeader* h) noexcept;
    AllocHeader* PrevAllocHeaderPtr(AllocHeader* h) noexcept;
    U64          GetAllocSmallBinIndexFromSize(U64 sz) noexcept;
    // Scheduling routines
    CThreadCtxHdl   ScheduleNextTask() noexcept;
    CThreadContext* get_linked_context(const utl::DLink* link) noexcept;
    
    // Debug routines
    Void DebugTaskCount() noexcept;
    Void DebugIOURingParams(const io_uring_params* p);
    Void DebugDumpAllocTable() noexcept;
    Void DebugPrintAllocBlocks() noexcept;
    
    // Invariant checking routines
    Void CheckInvariants() noexcept;
    
}} // namespace ak::priv

