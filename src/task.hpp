#pragma once

// -----------------------------------------------------------------------------
// Basic definitions
// -----------------------------------------------------------------------------

#include <coroutine>
#include <print>
#include <functional>
#include "dlist.hpp"
#include "defs.hpp"

struct Kernel;
struct Task;
struct TaskScheduler;
struct TaskPromise;
struct SchedulerTaskPromise;
struct SuspendEffect;
struct NopEffect;

using TaskHdl = std::coroutine_handle<TaskPromise>;
using CoroHdl = std::coroutine_handle<>;

template <typename... Args>
using TaskFn = std::function<Task(Args...)>;

enum class TaskState {
    CREATED,    // Task has been created (BUT NOT REGISTERED WITH THE RUNTINME)
    READY,      // Ready for execution
    RUNNING,    // Currently running
    IO_WAITING, // Waiting for IO 
    WAITING,    // Waiting for Critical Section
    ZOMBIE      // Already dead
};

// -----------------------------------------------------------------------------
// Task Promise
// -----------------------------------------------------------------------------

struct TaskPromise {
    
    struct FinalSuspend {
        constexpr bool await_ready() const noexcept { return false; }
        CoroHdl        await_suspend(TaskHdl hdl) const noexcept;
        constexpr void await_resume() const noexcept {
            std::printf("TaskPromise::FinalSuspend: illegal await_resume\n");
            assert(false);
        }
    };
    
    struct InitialSuspend {
        constexpr bool await_ready() const noexcept { return false; }
        void           await_suspend(TaskHdl hdl) const noexcept;
        constexpr void await_resume() const noexcept;
    };

    void* operator new(std::size_t n) noexcept;
    void  operator delete(void* ptr, std::size_t sz);

    TaskPromise();
    ~TaskPromise();
    
    TaskHdl get_return_object() noexcept;

    InitialSuspend initial_suspend() noexcept { return {}; }
    FinalSuspend   final_suspend() noexcept { return {}; }
    void           return_void() noexcept;
    void           unhandled_exception() noexcept { assert(false); }

    TaskState state;
    DList     wait_node; // Used to enqueue tasks waiting for Critical Section
    DList     task_list; // Global Task list
};

// -----------------------------------------------------------------------------
// Kernel 
// -----------------------------------------------------------------------------

struct Kernel {
    int     task_count;
    int     ready_count;      // number of live tasks 
    int     waiting_count;    // task waiting for execution (On Internal Critical sections)
    int     io_waiting_count; // task waiting for IO URing
    int     zombie_count;     // dead tasks

    int     interrupted;
    TaskHdl current_task_hdl;
    TaskHdl scheduler_task_hdl;
    DList   zombie_list;
    DList   ready_list;
    DList   task_list;  // global task list
};

extern struct Kernel g_kernel;

// -----------------------------------------------------------------------------
// Kernel API 
// -----------------------------------------------------------------------------

int startMainTask(std::function<Task()> user_main_task) noexcept;

// -----------------------------------------------------------------------------
// Debug & invariant Utilities
// -----------------------------------------------------------------------------

void checkTaskCountInvariant() noexcept;
void debugTaskCount() noexcept;

// -----------------------------------------------------------------------------
// Task Wrappers
// -----------------------------------------------------------------------------


struct ExecuteTaskEffect {

    ExecuteTaskEffect(TaskHdl hdl) : hdl(hdl) {};

    bool await_ready() const noexcept {
        return hdl.done();
    }

    TaskHdl await_suspend(TaskHdl currentTaskHdl) const noexcept;

    void await_resume() const noexcept {
        std::printf("Task(%p)::await_resume(): has returned\n", &hdl.promise()); 
    }

    TaskHdl hdl;
};

struct Task {
    using promise_type = TaskPromise;

    Task() = default;
    
    Task(TaskHdl hdl) : hdl(hdl) {}

    auto operator co_await() const noexcept {
        return ExecuteTaskEffect(hdl);
    }

    TaskState state() const;

    bool done() const noexcept {
        return hdl.done();
    }

    TaskHdl hdl;
};


// -----------------------------------------------------------------------------
// Utilities
// -----------------------------------------------------------------------------

static TaskPromise* waitListNodeToTask(DList* node);

// -----------------------------------------------------------------------------
// Effects
// -----------------------------------------------------------------------------

struct SuspendEffect {
    constexpr bool await_ready() const noexcept;
    TaskHdl        await_suspend(TaskHdl hdl) const noexcept;
    constexpr void await_resume() const noexcept;
};

constexpr auto suspend() noexcept-> SuspendEffect;


// ==============================================================================
// Inline implementation 
// ==============================================================================

inline TaskPromise* waitListNodeToTask(DList* node) {
    unsigned long long promise_off = ((unsigned long long)node) - offsetof(TaskPromise, wait_node);
    return (TaskPromise*)promise_off;
}

inline TaskState Task::state() const {  
    return hdl.promise().state;
}

inline constexpr auto suspend() noexcept-> SuspendEffect { return {}; }

inline constexpr void TaskPromise::InitialSuspend::await_resume() const noexcept {
    TaskPromise& current_task = g_kernel.current_task_hdl.promise();
    std::printf("TaskPromise::InitialSuspend::await_resume(): started Task(%p); state: %d\n", &current_task, current_task.state); 
}

//  SuspendEffect

inline constexpr bool SuspendEffect::await_ready() const noexcept { 
    std::printf("Suspend effect: is_ready? (always false)\n");
    return false; 
}

inline constexpr void SuspendEffect::await_resume() const noexcept {
    std::printf("SuspendEffect::await_resume(): resumed Task(%p)\n", &g_kernel.current_task_hdl.promise());
}

// -----

