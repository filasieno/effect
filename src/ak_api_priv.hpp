#pragma once

#include "ak_api.hpp"

namespace ak 
{

    namespace priv 
    {
        TaskHdl ScheduleNextTask() noexcept;
        void    CheckInvariants() noexcept;
        void    DebugTaskCount() noexcept;
    } // namespace priv

} // namespace ak
