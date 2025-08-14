#pragma once

#include "liburing.h"
#include <cstdint>

namespace ak {

using U64  = __u64;
using U32  = __u32;
using Size = __SIZE_TYPE__;

/// \brief Idenfies the state of a task
/// \ingroup Task
enum class TaskState
{
    INVALID = 0, ///< Invalid OR uninitialized state
    CREATED,     ///< Task has been created (BUT NOT REGISTERED WITH THE RUNTINME)
    READY,       ///< Ready for execution
    RUNNING,     ///< Currently running
    IO_WAITING,  ///< Waiting for IO
    WAITING,     ///< Waiting for Critical Section
    ZOMBIE,      ///< Already dead
    DELETING     ///< Currently being deleted
};

inline const char* ToString(TaskState state) noexcept 
{
    switch (state) {
        case TaskState::INVALID:    return "INVALID";
        case TaskState::CREATED:    return "CREATED";
        case TaskState::READY:      return "READY";
        case TaskState::RUNNING:    return "RUNNING";
        case TaskState::IO_WAITING: return "IO_WAITING";
        case TaskState::WAITING:    return "WAITING";
        case TaskState::ZOMBIE:     return "ZOMBIE";
        case TaskState::DELETING:   return "DELETING";
        default: return nullptr;
    }
}

} // namespace ak



