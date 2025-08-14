#pragma once

#include "ak/api.hpp"

namespace ak { namespace priv {

    // Allocator routines
    int   InitAllocTable(void* mem, Size size) noexcept;
    void* TryMalloc(Size size) noexcept;
    void  FreeMem(void* ptr, unsigned sideCoalescing = UINT_MAX) noexcept;

    // Scheduling routines
    TaskHdl ScheduleNextTask() noexcept;
    
    // Debug routines
    void    DebugTaskCount() noexcept;
    void    DebugIOURingParams(const io_uring_params* p);
    void    DebugDumpAllocTable(AllocTable* at) noexcept;

    // Invariant checking routines
    void    CheckInvariants() noexcept;
    
}} // namespace ak::priv

