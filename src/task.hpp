#pragma once

#include <cassert>
#include <coroutine>
#include <print>
#include <functional>
#include "defs.hpp"

// -----------------------------------------------------------------------------
// Basic definitions
// -----------------------------------------------------------------------------

struct Kernel;
struct Task;
struct TaskScheduler;
struct TaskPromise;
struct SchedulerTaskPromise;
struct SuspendOp;
struct NopEffect;
struct KernelBootPromise;

using CoroHdl           = std::coroutine_handle<>;
using TaskHdl           = std::coroutine_handle<TaskPromise>;
using KernelBootTaskHdl = std::coroutine_handle<KernelBootPromise>;

template <typename... Args>
using TaskFn = std::function<Task(Args...)>;

enum class TaskState 
{
    INVALID = 0,
    CREATED,    // Task has been created (BUT NOT REGISTERED WITH THE RUNTINME)
    READY,      // Ready for execution
    RUNNING,    // Currently running
    IO_WAITING, // Waiting for IO 
    WAITING,    // Waiting for Critical Section
    ZOMBIE,     // Already dead 
    DELETING
};

// -----------------------------------------------------------------------------
// Utils
// -----------------------------------------------------------------------------
namespace internal_ak {

    struct Link {
        Link* next;
        Link* prev;
    };

    inline void InitLink(Link* dlist) {
        dlist->next = dlist;
        dlist->prev = dlist;
    }

    inline bool IsLinkDetached(const Link* dlist) {
        assert(dlist->next != nullptr);
        assert(dlist->prev != nullptr);
        return dlist->next == dlist && dlist->prev == dlist;
    }

    inline void DetachLink(Link* dlist) {
        assert(dlist->next != nullptr);
        assert(dlist->prev != nullptr);
        if (IsLinkDetached(dlist)) return;
        dlist->next->prev = dlist->prev;
        dlist->prev->next = dlist->next;
        dlist->next = dlist;
        dlist->prev = dlist;
    }

    inline void EnqueueLink(Link* dlist, Link* node) {
        assert(node != nullptr);
        assert(dlist->next != nullptr);
        assert(dlist->prev != nullptr);
        assert(IsLinkDetached(node));

        node->next = dlist->next;
        node->prev = dlist;
        
        node->next->prev = node;
        dlist->next = node;
    }

    inline Link* DequeueLink(Link* dlist) {
        assert(dlist->next != nullptr);
        assert(dlist->prev != nullptr);
        if (IsLinkDetached(dlist)) return nullptr;
        Link* target = dlist->prev;
        DetachLink(target);
        return target;
    }

}



// -----------------------------------------------------------------------------
// Task Promise
// -----------------------------------------------------------------------------

struct TaskPromise {
    

    void* operator new(std::size_t n) noexcept;
    void  operator delete(void* ptr, std::size_t sz);

    struct InitialSuspend {
        constexpr bool await_ready() const noexcept { return false; }
        void           await_suspend(TaskHdl hdl) const noexcept;
        constexpr void await_resume() const noexcept;
    };

    struct FinalSuspend {
        constexpr bool await_ready() const noexcept { return false; }
        TaskHdl        await_suspend(TaskHdl hdl) const noexcept;
        constexpr void await_resume() const noexcept {
            std::print("TaskPromise::FinalSuspend: illegal await_resume\n");
            assert(false);
        }
    };
    
    TaskPromise();
    ~TaskPromise();
    
    TaskHdl        get_return_object() noexcept;
    InitialSuspend initial_suspend() noexcept { return {}; }
    FinalSuspend   final_suspend() noexcept { return {}; }
    void           return_void() noexcept;
    void           unhandled_exception() noexcept { assert(false); }

    TaskState            state;
    internal_ak::Link waitNode;        // Used to enqueue tasks waiting for Critical Section
    internal_ak::Link taskList;        // Global Task list
    internal_ak::Link waitTaskNode; // The list of all tasks waiting for this task
};

struct ResumeTaskOp {

    explicit ResumeTaskOp(TaskHdl hdl) : hdl(hdl) {};

    constexpr bool await_ready() const noexcept { return hdl.done();}
    TaskHdl        await_suspend(TaskHdl currentTaskHdl) const noexcept;
    constexpr void await_resume() const noexcept {}

    TaskHdl hdl;
};

struct JoinTaskOp {
    explicit JoinTaskOp(TaskHdl hdl);            
    constexpr bool await_ready() const noexcept;
    constexpr TaskHdl await_suspend(TaskHdl currentTaskHdl) const noexcept;
    constexpr void await_resume() const noexcept;

    TaskHdl hdl;
};

struct Task {
    using promise_type = TaskPromise;

    Task() = default;
    Task(const TaskHdl& hdl) : hdl(hdl) {}
    Task& operator=(const Task& hdl) = default; 
    
    void clear() noexcept {
        hdl = TaskHdl{};
    }

    bool isValid() const noexcept {
        return hdl.address() != nullptr;
    }
    
    bool done() const noexcept {
        return hdl.done();
    }

    TaskState state() const noexcept {
        return hdl.promise().state;
    }

    TaskPromise& promise() const noexcept {
        return hdl.promise();
    }

    operator TaskHdl() const noexcept {
        return hdl;
    }

    auto operator co_await() const noexcept {
        return join();
    }

    JoinTaskOp join() const noexcept {
        return JoinTaskOp{hdl};
    }

    ResumeTaskOp resume() const noexcept {
        return ResumeTaskOp{hdl};
    }


    TaskHdl hdl;
};

inline bool operator==(const Task& lhs, const Task& rhs) noexcept {   
    return lhs.hdl.address() == rhs.hdl.address();
}

// -----------------------------------------------------------------------------
// Kernel 
// -----------------------------------------------------------------------------

struct Kernel {
    int   taskCount;
    int   readyCount;      // number of live tasks 
    int   waitingCount;    // task waiting for execution (On Internal Critical sections)
    int   ioWaitingCount;  // task waiting for IO URing
    int   zombieCount;     // dead tasks

    int   interrupted;
    Task  currentTask;
    Task  schedulerTask;
    
    internal_ak::Link zombieList;
    internal_ak::Link readyList;
    internal_ak::Link taskList;        // global task list

    KernelBootTaskHdl kernelTask;
};

extern struct Kernel gKernel;

// -----------------------------------------------------------------------------
// Kernel API 
// -----------------------------------------------------------------------------

int AkRun(std::function<Task()> userMainTask) noexcept;

// -----------------------------------------------------------------------------
// Debug & invariant Utilities
// -----------------------------------------------------------------------------

namespace internal_ak {
    void CheckInvariants() noexcept;
    void DebugTaskCount() noexcept;
}

// -----------------------------------------------------------------------------
// Task Wrappers
// -----------------------------------------------------------------------------

struct Condition {
    

    struct WaitOp {
        
        explicit WaitOp(Condition& condition) : condition(condition) {}
        
        constexpr bool await_ready() const noexcept {
            return condition.locking_task.isValid();
        }

        constexpr TaskHdl await_suspend(TaskHdl currentTaskHdl) const noexcept {
            using namespace internal_ak;

            TaskPromise& currentTaskPromise = gKernel.currentTask.promise();

            assert(gKernel.currentTask == currentTaskHdl);

            std::print("Condition::WaitEffect::await_suspend: requested to block Task({})\n", (void*)&currentTaskHdl.promise());
            
            // Check the current Task
            
            assert(IsLinkDetached(&currentTaskPromise.waitNode));
            assert(currentTaskPromise.state == TaskState::RUNNING);
            CheckInvariants();

            // Move the current task from READY to WAITING into the condition
            currentTaskPromise.state = TaskState::WAITING;
            ++gKernel.waitingCount;
            EnqueueLink(&condition.waitNode, &currentTaskPromise.waitNode);
            gKernel.currentTask.clear();
            CheckInvariants();

            // Move the target task from READY to RUNNING
            TaskPromise& schedulerPromise = gKernel.schedulerTask.promise();
            schedulerPromise.state = TaskState::RUNNING;
            DetachLink(&schedulerPromise.waitNode); // remove from ready list
            --gKernel.readyCount;
            gKernel.currentTask = gKernel.schedulerTask;
            CheckInvariants();
            return gKernel.schedulerTask;
        }

        constexpr void await_resume() const noexcept {}

        Condition& condition;
    };

    Condition(bool signaled = false) : signaled(signaled) {
        using namespace internal_ak;
        InitLink(&waitNode);
    }

    int signal() {
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
    
    WaitOp wait() { return WaitOp(*this); }
    
    void reset(bool signaled = false) {
        using namespace internal_ak;
        this->signaled = signaled;
        locking_task.clear();
        InitLink(&waitNode);
    }

    // g_kernel.current_task_hdl = TaskHdl();
    // g_kernel.current_task.clear();
    bool  signaled = false;
    Task  locking_task;
    internal_ak::Link waitNode;
};

// -----------------------------------------------------------------------------
// Utilities
// -----------------------------------------------------------------------------
namespace internal_ak {
    static TaskPromise* waitListNodeToTaskPromise(const Link* node) noexcept;
}

// -----------------------------------------------------------------------------
// Effects
// -----------------------------------------------------------------------------

struct SuspendOp {
    constexpr bool await_ready() const noexcept;
    TaskHdl        await_suspend(TaskHdl hdl) const noexcept;
    constexpr void await_resume() const noexcept;
};

constexpr auto suspend() noexcept-> SuspendOp;

namespace effect_internal {

    struct GetCurrentTaskOp {
        constexpr bool    await_ready() const noexcept        { return false;                } 
        constexpr TaskHdl await_suspend(TaskHdl hdl) noexcept { this->hdl = hdl; return hdl; }
        constexpr Task    await_resume() const noexcept       { return hdl;                  }
        
        TaskHdl hdl;
    };

}

inline auto getCurrentTask() noexcept {
    return effect_internal::GetCurrentTaskOp{};
}

// ==============================================================================
// Inline implementation 
// ==============================================================================
namespace internal_ak {
    inline TaskPromise* waitListNodeToTaskPromise(const Link* node) noexcept {
        unsigned long long promise_off = ((unsigned long long)node) - offsetof(TaskPromise, waitNode);
        return (TaskPromise*)promise_off;
    }
}

inline constexpr auto suspend() noexcept-> SuspendOp { return {}; }

inline constexpr void TaskPromise::InitialSuspend::await_resume() const noexcept {
    TaskPromise& current_task = gKernel.currentTask.promise();
    std::print("TaskPromise::InitialSuspend::await_resume(): started Task({}); state: {}\n", (void*)&current_task, (int)current_task.state); 
    std::fflush(stdout);
}

//  SuspendEffect

inline constexpr bool SuspendOp::await_ready() const noexcept { 
    std::print("Suspend effect: is_ready? (always false)\n");
    return false; 
}

inline constexpr void SuspendOp::await_resume() const noexcept {
    std::print("SuspendEffect::await_resume(): resumed Task({})\n", (void*)&gKernel.currentTask.promise());
}

// Task::AwaitTaskEffect
// ----------------------------------------------------------------------------------------------------------------


inline JoinTaskOp::JoinTaskOp(TaskHdl hdl) : hdl(hdl) {}
            
inline constexpr bool JoinTaskOp::await_ready() const noexcept 
{ 
    return hdl.done(); 
}

inline constexpr TaskHdl JoinTaskOp::await_suspend(TaskHdl currentTaskHdl) const noexcept 
{ 
    using namespace internal_ak;

    TaskPromise& currentTaskPromise = currentTaskHdl.promise();
    TaskPromise& hdlTaskPromise = hdl.promise();

    // Check preconditions

    assert(currentTaskPromise.state == TaskState::RUNNING);
    assert(IsLinkDetached(&currentTaskPromise.waitNode));
    assert(gKernel.currentTask == currentTaskHdl);
    CheckInvariants();

    // Move the current task from RUNNINIG to WAITING

    currentTaskPromise.state = TaskState::WAITING;
    ++gKernel.waitingCount;
    EnqueueLink(&hdlTaskPromise.waitNode, &currentTaskPromise.waitNode);
    gKernel.currentTask.clear();
    CheckInvariants();
    std::print("JoinTaskOp::await_suspend(): Task({}) is waiting on Task({}) termination\n", (void*)&currentTaskPromise, (void*)&hdlTaskPromise); 
    DebugTaskCount();
    // if (hdlTaskPromise.state == TaskState::READY) {
    //     // Just move to READY
    //     return hdlTaskPromise;
    // }

    // Move the target task from READY to RUNNING
    TaskPromise& schedulerPromise = gKernel.schedulerTask.promise();
    assert(schedulerPromise.state == TaskState::READY);
    schedulerPromise.state = TaskState::RUNNING;
    DetachLink(&schedulerPromise.waitNode);
    --gKernel.readyCount;
    gKernel.currentTask = gKernel.schedulerTask;
    CheckInvariants();
    DebugTaskCount();

    return gKernel.schedulerTask; 
}

inline constexpr void JoinTaskOp::await_resume() const noexcept {}


inline TaskHdl SuspendOp::await_suspend(TaskHdl currentTask) const noexcept {
    using namespace internal_ak;
    assert(gKernel.currentTask.isValid());

    TaskPromise& currentPromise = currentTask.promise();

    if constexpr (DEFINED_DEBUG) {
        assert(gKernel.currentTask == currentTask);  
        assert(currentPromise.state == TaskState::RUNNING);
        assert(IsLinkDetached(&currentPromise.waitNode));
        CheckInvariants();
    }
    
    std::print("SuspendEffect: suspending task({})\n", (void*)&gKernel.currentTask.promise());
    std::print("Task({}): scheduleNextTask() suspending the current task\n", (void*)&currentPromise); 

    // Move the current task from RUNNINIG to READY
    currentPromise.state = TaskState::READY;
    ++gKernel.readyCount;
    EnqueueLink(&gKernel.readyList, &currentPromise.waitNode);
    gKernel.currentTask.clear();
    CheckInvariants();
    
    // Resume the SchedulerTask
    TaskPromise& schedulerPromise = gKernel.schedulerTask.promise();
    schedulerPromise.state = TaskState::RUNNING;
    DetachLink(&schedulerPromise.waitNode); // remove from ready list
    --gKernel.readyCount;
    gKernel.currentTask = gKernel.schedulerTask;
    CheckInvariants();

    assert(gKernel.currentTask.isValid());
    return TaskHdl::from_promise(schedulerPromise);
}

inline TaskHdl ResumeTaskOp::await_suspend(TaskHdl currentTaskHdl) const noexcept {
    using namespace internal_ak;
    assert(gKernel.currentTask == currentTaskHdl);
    
    std::print("Task({0})::resume(): requested to resume Task({1})\n", (void*)&gKernel.currentTask.promise(), (void*)&hdl.promise());
    
    // Check the current Task
    TaskPromise& currentPromise = gKernel.currentTask.promise(); 
    assert(IsLinkDetached(&currentPromise.waitNode));
    assert(currentPromise.state == TaskState::RUNNING); 
    CheckInvariants();

    // Suspend the current Task
    currentPromise.state = TaskState::READY;
    ++gKernel.readyCount;
    EnqueueLink(&gKernel.readyList, &currentPromise.waitNode);
    gKernel.currentTask = TaskHdl();
    CheckInvariants();

    // Move the target task from READY to RUNNING
    TaskPromise& promise = hdl.promise();
    promise.state = TaskState::RUNNING;
    DetachLink(&promise.waitNode);
    --gKernel.readyCount;
    gKernel.currentTask = hdl;
    CheckInvariants();

    assert(gKernel.currentTask.isValid());
    return hdl;
}