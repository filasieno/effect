-- Formaward definitions & includes

--- Basic 

struct DList
struct TaskHdl
struct CoroHdl
enum class TaskState

--- Key Promise
struct TaskPromise
- struct TaskPromise::InitialSuspend
- struct TaskPromise::FinalSuspend

--- Kernel
struct Kernel

--- Kernel API
api Kernel
api Debug

-- Pimpl
struct Task
struct SchedulerTask

-- Effects that use the kernel
struct SuspendEffect

== Physical layout

- task.hpp
- task_internal.hpp

- kernel.cc   : io
- task.cc     : task + taskpromise
- effects.cc
