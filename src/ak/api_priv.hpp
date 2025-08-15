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
    int          InitAllocTable(void* mem, Size size) noexcept;
    AllocHeader* NextAllocHeaderPtr(AllocHeader* h) noexcept;
    AllocHeader* PrevAllocHeaderPtr(AllocHeader* h) noexcept;
    U64          GetAllocSmallBinIndexFromSize(U64 sz) noexcept;
    // Scheduling routines
    TaskHdl      ScheduleNextTask() noexcept;
    TaskContext* GetLinkedTaskContext(const utl::DLink* link) noexcept;
    
    // Debug routines
    void DebugTaskCount() noexcept;
    void DebugIOURingParams(const io_uring_params* p);
    void DebugDumpAllocTable() noexcept;
    void DebugPrintAllocBlocks() noexcept;
    
    // Invariant checking routines
    void CheckInvariants() noexcept;
    
}} // namespace ak::priv

