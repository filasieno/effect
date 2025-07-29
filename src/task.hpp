#pragma once


#include <coroutine>
#include <print>
#include <functional>
#include "dlist.hpp"
#include "defs.hpp"

// -----------------------------------------------------------------------------
// Basic definitions
// -----------------------------------------------------------------------------

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

int runMainTask(std::function<Task()> user_main_task) noexcept;

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

struct Condition {
    struct WaitEffect {
        
        explicit WaitEffect(Condition& condition) : condition(condition) {}
        
        constexpr bool await_ready() const noexcept {
            return condition.hdl == TaskHdl();
        }

        constexpr TaskHdl await_suspend(TaskHdl currentTaskHdl) const noexcept {
            assert(g_kernel.current_task_hdl == currentTaskHdl);
            
            std::print("Condition::WaitEffect::await_suspend: requested to block Task({})\n", (void*)&currentTaskHdl.promise());
            
            // // Check the current Task
            // TaskPromise& current_task_promise = g_kernel.current_task_hdl.promise(); 
            // assert(current_task_promise.wait_node.detached());
            // assert(current_task_promise.state == TaskState::RUNNING); 
            // checkTaskCountInvariant();

            // // Block the current Task
            // current_task_promise.state = TaskState::WAITING;
            // ++g_kernel.waiting_count;
            // condition.wait_node.push_back(&current_task_promise.wait_node);
            // g_kernel.current_task_hdl = TaskHdl();
            // checkTaskCountInvariant();

            // // Move the target task from READY to RUNNING
            // TaskPromise& target_task_promise = g_kernel.scheduler_task_hdl.promise();
            // target_task_promise.state = TaskState::RUNNING;
            // target_task_promise.wait_node.detach();
            // --g_kernel.ready_count;
            // g_kernel.current_task_hdl = hdl;
            // checkTaskCountInvariant();
            return currentTaskHdl;
        }

        constexpr void await_resume() const noexcept {}

        Condition& condition;
    };

    Condition() {
        wait_node.init();
    }

    int signalAll() {
        int signalled = 0;
        // while (!wait_node.detached()) {
        //     DList* next_waiting_task = wait_node.pop_front();
        //     TaskPromise* next_waiting_task_promise = waitListNodeToTask(next_waiting_task);
        //     assert(next_waiting_task_promise->state == TaskState::WAITING);
        //     --g_kernel.waiting_count;
        //     next_waiting_task_promise->state = TaskState::READY;
        //     g_kernel.ready_list.push_back(&next_waiting_task_promise->wait_node);
        //     --g_kernel.ready_count;
        //     ++signalled;
        // }
        return signalled;
    }
    
    void reset() {
        hdl = TaskHdl();
        wait_node.init();
    }

    WaitEffect wait() { return WaitEffect(*this); }

    TaskHdl hdl;
    DList   wait_node;
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


struct GetCurrentTaskEffect {
    constexpr bool    await_ready() const noexcept        { return false;                } 
    constexpr TaskHdl await_suspend(TaskHdl hdl) noexcept { this->hdl = hdl; return hdl; }
    constexpr TaskHdl await_resume() const noexcept       { return hdl;                  }
    
    TaskHdl hdl;
};

inline GetCurrentTaskEffect getCurrentTask() noexcept {
    return {};
}

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
    std::fflush(stdout);
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

