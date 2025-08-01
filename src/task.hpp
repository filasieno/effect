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

struct DList {

    void init() {
        this->next = this;
        this->prev = this;
    }

    void detach() {
        assert(this->next != nullptr);
        assert(this->prev != nullptr);
        if (detached()) return;
        this->next->prev = this->prev;
        this->prev->next = this->next;
        this->next = this;
        this->prev = this;
    }
    
    void pushFront(DList* node) {
        assert(node != nullptr);
        assert(this->next != nullptr);
        assert(this->prev != nullptr);
        assert(node->detached());
    
        node->prev = this->prev;
        node->next = this;
        
        node->prev->next = node;
        this->prev = node; 
    }

    void pushBack(DList* node) {
        assert(node != nullptr);
        assert(this->next != nullptr);
        assert(this->prev != nullptr);
        assert(node->detached());

        node->next = this->next;
        node->prev = this;
        
        node->next->prev = node;
        this->next = node;
    }

    DList* popFront() {
        assert(this->next != nullptr);
        assert(this->prev != nullptr);
        if (detached()) return nullptr;
        DList* target = this->prev;
        target->detach();
        return target;
    }

    DList* popBack() {
        assert(this->next != nullptr);
        assert(this->prev != nullptr);
        if (detached()) return nullptr;
        DList* target = this->next;
        target->detach();
        return target;
    }

    DList& front() {
        assert(this->next != nullptr);
        assert(this->prev != nullptr);
        return *this->prev;
    }

    DList& back() {
        assert(this->next != nullptr);
        assert(this->prev != nullptr);
        return *this->next;        
    }
    
    bool detached() const {
        assert(this->next != nullptr);
        assert(this->prev != nullptr);
        return this->next == this && this->prev == this;
    }

    DList* next;
    DList* prev;
};

// -----------------------------------------------------------------------------
// Task Promise
// -----------------------------------------------------------------------------

// struct Event {
//     void signal() noexcept {

//     }

//     void wait() noexcept {
        
//     }
//     DList node;
// }

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

    TaskState state;
    DList     waitNode;        // Used to enqueue tasks waiting for Critical Section
    DList     taskList;        // Global Task list
    DList     terminatedEvent; // The list of all tasks waiting for this task
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
    
    DList zombieList;
    DList readyList;
    DList taskList;        // global task list

    KernelBootTaskHdl kernelTask;
};

extern struct Kernel gKernel;

// -----------------------------------------------------------------------------
// Kernel API 
// -----------------------------------------------------------------------------

int runMainTask(std::function<Task()> userMainTask) noexcept;

// -----------------------------------------------------------------------------
// Debug & invariant Utilities
// -----------------------------------------------------------------------------

void checkInvariants() noexcept;
void debugTaskCount() noexcept;

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
            TaskPromise& currentTaskPromise = gKernel.currentTask.promise();

            assert(gKernel.currentTask == currentTaskHdl);

            std::print("Condition::WaitEffect::await_suspend: requested to block Task({})\n", (void*)&currentTaskHdl.promise());
            
            // Check the current Task
            
            assert(currentTaskPromise.waitNode.detached());
            assert(currentTaskPromise.state == TaskState::RUNNING);
            checkInvariants();

            // Move the current task from READY to WAITING into the condition
            currentTaskPromise.state = TaskState::WAITING;
            ++gKernel.waitingCount;
            condition.waitNode.pushBack(&currentTaskPromise.waitNode);
            gKernel.currentTask.clear();
            checkInvariants();

            // Move the target task from READY to RUNNING
            TaskPromise& schedulerPromise = gKernel.schedulerTask.promise();
            schedulerPromise.state = TaskState::RUNNING;
            schedulerPromise.waitNode.detach(); // remove from ready list
            --gKernel.readyCount;
            gKernel.currentTask = gKernel.schedulerTask;
            checkInvariants();
            return gKernel.schedulerTask;
        }

        constexpr void await_resume() const noexcept {}

        Condition& condition;
    };

    Condition(bool signaled = false) : signaled(signaled) {
        waitNode.init();
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
        this->signaled = signaled;
        locking_task.clear();
        waitNode.init();
    }

    // g_kernel.current_task_hdl = TaskHdl();
    // g_kernel.current_task.clear();
    bool  signaled = false;
    Task  locking_task;
    DList waitNode;
};

// -----------------------------------------------------------------------------
// Utilities
// -----------------------------------------------------------------------------

static TaskPromise* waitListNodeToTaskPromise(const DList* node) noexcept;

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

inline TaskPromise* waitListNodeToTaskPromise(const DList* node) noexcept {
    unsigned long long promise_off = ((unsigned long long)node) - offsetof(TaskPromise, waitNode);
    return (TaskPromise*)promise_off;
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
    TaskPromise& currentTaskPromise = currentTaskHdl.promise();
    TaskPromise& hdlTaskPromise = hdl.promise();

    // Check preconditions

    assert(currentTaskPromise.state == TaskState::RUNNING);
    assert(currentTaskPromise.waitNode.detached());
    assert(gKernel.currentTask == currentTaskHdl);
    checkInvariants();

    // Move the current task from RUNNINIG to WAITING

    currentTaskPromise.state = TaskState::WAITING;
    ++gKernel.waitingCount;
    hdlTaskPromise.terminatedEvent.pushBack(&currentTaskPromise.waitNode);
    gKernel.currentTask.clear();
    checkInvariants();
    std::print("JoinTaskOp::await_suspend(): Task({}) is waiting on Task({}) termination\n", (void*)&currentTaskPromise, (void*)&hdlTaskPromise); 
    debugTaskCount();
    // if (hdlTaskPromise.state == TaskState::READY) {
    //     // Just move to READY
    //     return hdlTaskPromise;
    // }

    // Move the target task from READY to RUNNING
    TaskPromise& schedulerPromise = gKernel.schedulerTask.promise();
    assert(schedulerPromise.state == TaskState::READY);
    schedulerPromise.state = TaskState::RUNNING;
    schedulerPromise.waitNode.detach(); 
    --gKernel.readyCount;
    gKernel.currentTask = gKernel.schedulerTask;
    checkInvariants();
    debugTaskCount();
    
    return gKernel.schedulerTask; 
}

inline constexpr void JoinTaskOp::await_resume() const noexcept {}

