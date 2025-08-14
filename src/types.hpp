#pragma once
#include <coroutine>

namespace ak {

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

struct DefineTask;

struct TaskContext;

/// \brief Coroutine handle for a Task
/// \ingroup Task
using TaskHdl = std::coroutine_handle<TaskContext>;

/// \brief Defines a Task function type-erased pointer (no std::function)
/// \ingroup Task
template <typename... Args>
using TaskFn = DefineTask(*)(Args...);

namespace priv {
    struct KernelTaskPromise;
    using KernelTaskHdl = std::coroutine_handle<KernelTaskPromise>;
} // namespace priv

} // namespace ak
