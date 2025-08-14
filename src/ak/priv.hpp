#pragma once

#include "ak_api.hpp"

namespace ak 
{

    namespace priv 
    {
        // Allocator routines
        int   InitAllocTable(AllocTable* at, void* mem, Size size) noexcept;
        void* TryMalloc(AllocTable* at, Size size) noexcept;
        void  FreeMem(AllocTable* at, void* ptr, unsigned sideCoalescing = UINT_MAX) noexcept;


        // Scheduling routines
        TaskHdl ScheduleNextTask() noexcept;
        
        // Debug routines
        void    DebugTaskCount() noexcept;
        void    DebugIOURingParams(const io_uring_params* p);
        void    DebugDumpAllocTable(AllocTable* at) noexcept;

        // Invariant checking routines
        void    CheckInvariants() noexcept;
    } // namespace priv

} // namespace ak

