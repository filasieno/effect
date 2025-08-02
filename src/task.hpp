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




static void initKernel() noexcept;
static void finiKernel() noexcept; 

struct RunSchedulerTaskOp {

    RunSchedulerTaskOp(TaskHdl schedulerHdl) : schedulerHdl(schedulerHdl) {};

    bool await_ready() const noexcept {
        return schedulerHdl.done();
    }

    TaskHdl await_suspend(KernelBootTaskHdl currentTaskHdl) const noexcept {
        using namespace internal_ak;

        (void)currentTaskHdl;
        TaskPromise& schedulerPromise = schedulerHdl.promise();
        std::print("RunSchedulerTaskOp::await_suspend: about to suspend KernelBootTaskHdl({0})\n", (void*)&currentTaskHdl.promise()); 
        
        // Check expected state post scheduler construction
        
        assert(gKernel.taskCount == 1);
        assert(gKernel.readyCount == 1);
        assert(schedulerPromise.state == TaskState::READY);
        assert(!IsLinkDetached(&schedulerPromise.waitNode));
        assert(gKernel.currentTask == TaskHdl());

        // Setup SchedulerTask for execution (from READY -> RUNNING)
        gKernel.currentTask = schedulerHdl;
        schedulerPromise.state = TaskState::RUNNING;
        DetachLink(&schedulerPromise.waitNode);
        --gKernel.readyCount;
    
        // Check expected state post task system bootstrap
        CheckInvariants();
        std::print("RunSchedulerTaskOp::await_suspend: about to resume Task({0})\n", (void*)&schedulerHdl.promise());
        return schedulerHdl;
    }

    void await_resume() const noexcept {
        std::print("RunSchedulerTaskOp: await_resume\n");
    }

    TaskHdl schedulerHdl;
};

struct TerminateSchedulerOp {
    constexpr bool await_ready() const noexcept { return false; }
    KernelBootTaskHdl await_suspend(TaskHdl hdl) const noexcept { 
        using namespace internal_ak;
        std::print(">> TerminateSchedulerOp: about to terminate the scheduler task\n");         
        assert(gKernel.currentTask == gKernel.schedulerTask);
        assert(gKernel.currentTask == hdl);

        TaskPromise& schedulerPromise = gKernel.schedulerTask.promise();
        assert(schedulerPromise.state == TaskState::RUNNING);
        assert(IsLinkDetached(&schedulerPromise.waitNode));
        
        schedulerPromise.state = TaskState::ZOMBIE;
        gKernel.currentTask.clear();
        EnqueueLink(&gKernel.zombieList, &schedulerPromise.waitNode);
        ++gKernel.zombieCount;
        
        std::print(">> TerminateSchedulerOp: did terminate the scheduler task\n");         
        return gKernel.kernelTask;
    }
    constexpr void await_resume() const noexcept {}
};

struct SchedulerTask {
    using promise_type = TaskPromise;
    
    SchedulerTask(TaskHdl hdl) noexcept : hdl(hdl) {}
    ~SchedulerTask() noexcept { 
        TaskPromise& schedulerPromise = hdl.promise();
        
        // Remove from Task list
        DetachLink(&schedulerPromise.taskList);
        --gKernel.taskCount;

        // Remove from Zombie List
        DetachLink(&schedulerPromise.waitNode);
        --gKernel.zombieCount;

        schedulerPromise.state = TaskState::DELETING;
        hdl.destroy();

    }
    bool done() const noexcept { return hdl.done(); }
    RunSchedulerTaskOp run() const noexcept { return RunSchedulerTaskOp{hdl}; }
    TaskState state() const noexcept;

    TaskHdl hdl;
};

struct KernelBootPromise {

    KernelBootPromise(std::function<Task()> userMainTask) : userMainTask(userMainTask){
        std::print("KernelBootPromise({0}): initialized\n", (void*)this);  
    }

    ~KernelBootPromise() {
        std::print("KernelBootPromise({0}): finalized\n", (void*)this);  
    }
    
    KernelBootTaskHdl get_return_object() noexcept {
        std::print("KernelBootPromise({0})::get_return_object: returning handle\n", (void*)this);     
        return KernelBootTaskHdl::from_promise(*this);
    }

    std::suspend_always initial_suspend() noexcept { 
        std::print("KernelBootPromise({0})::initial_suspend()\n", (void*)this);    
        return {}; 
    }

    std::suspend_never final_suspend() noexcept { 
        std::print("KernelBootPromise({0})::final_suspend()\n", (void*)this);    
        return {}; 
    }

    void return_void() noexcept {
        std::print("KernelBootPromise({0})::return_void(): Kernel Task has returned\n", (void*)this);    
    }

    void unhandled_exception() noexcept { assert(false); }

    std::function<Task()> userMainTask;
};

struct KernelBootTask {
    using promise_type = KernelBootPromise;
    
    KernelBootTask(KernelBootTaskHdl hdl) noexcept : hdl(hdl) {}
    ~KernelBootTask() noexcept { 
        // hdl.destroy();
    }
    void run() { hdl.resume();}

    KernelBootTaskHdl hdl;
};

static KernelBootTask mainKernelTask(std::function<Task()> userMainTask) noexcept;
static SchedulerTask  schedulerTask(std::function<Task()> userMainTask) noexcept;

inline int AkRun(std::function<Task()> userMainTask) noexcept {
    using namespace internal_ak;
    std::setbuf(stdout, nullptr);
    std::setbuf(stderr, nullptr); 

    std::print("runMainTask(): started ...\n"); 

    initKernel();

    std::print("runMainTask(): About to create the KernelBootTask\n");  
    KernelBootTask kernelBootTask = mainKernelTask(userMainTask);
    gKernel.kernelTask = kernelBootTask.hdl;
    std::print("runMainTask(): Did create the KernelBootTask({0})\n", (void*)&kernelBootTask.hdl.promise());  
    std::print("runMainTask(): About to run the KernelBootTask({0})\n", (void*)&kernelBootTask.hdl.promise());      
    kernelBootTask.run();
    std::print("runMainTask(): did run the KernelBootTask({0})\n", (void*)&kernelBootTask.hdl.promise());       
        
    finiKernel();

    std::print("runMainTask(): terminated\n");  
    return 0; 
}

inline KernelBootTask mainKernelTask(std::function<Task()> userMainTask) noexcept {
    using namespace internal_ak;

    std::print("mainKernelTask(std::function<Task): started the main kernel task\n"); 
    
    // Spawn the SchedulerTask
    std::print("mainKernelTask(std::function<Task): about to spawn the SchedulerTask\n"); 
    SchedulerTask task = schedulerTask(userMainTask);
    gKernel.schedulerTask = task.hdl;
    std::print("mainKernelTask(std::function<Task): did spawn SchedulerTask({0})\n", (void*)&task.hdl.promise()); 

    std::print("mainKernelTask(std::function<Task): about to execute SchedulerTask({0})\n", (void*)&task.hdl.promise());  
    co_await task.run();
    std::print("mainKernelTask(std::function<Task): did to execute SchedulerTask({0})\n", (void*)&task.hdl.promise());  
    DebugTaskCount();
    
    std::print("mainKernelTask(std::function<Task): terminated the main kernel task\n"); 
    co_return;
}

static int started = 0;

inline SchedulerTask schedulerTask(std::function<Task()> userMainTask) noexcept {   
    using namespace internal_ak;

    ++started;
    std::fflush(stdout);
    assert(started == 1);

    std::print(">> SchedulerTask({}): started\n", (void*)&gKernel.currentTask.promise());
    std::print(">> SchedulerTask({}): about to spawn main task\n", (void*)&gKernel.currentTask.promise()); // g_kernel.current_task_promise 
    Task mainTask = userMainTask();
    std::print(">> SchedulerTask({}): Main task is Task({})\n", (void*)&gKernel.currentTask.promise(), (void*)&mainTask.hdl.promise());
    assert(!mainTask.hdl.done());
    assert(mainTask.state() == TaskState::READY);
    std::print(">> SchedulerTask({}): did spawn main task\n", (void*)&gKernel.currentTask.promise());
    
    DebugTaskCount();

    while (true) {
        std::print(">> SchedulerTask({}): begin scheduler loop\n", (void*)&gKernel.currentTask.promise());

        // If we have a ready task, resume it
        std::print(">> SchedulerTask({}): ready count: {}\n", (void*)&gKernel.currentTask.promise(), gKernel.readyCount);
        if (gKernel.readyCount > 0) {
            std::print(">> SchedulerTask({}): ready count {} > 0\n", (void*)&gKernel.currentTask.promise(), gKernel.readyCount);
            Link* nextNode = gKernel.readyList.prev;
            TaskPromise& nextPromise = *waitListNodeToTaskPromise(nextNode);
            Task nextTask = TaskHdl::from_promise(nextPromise);
            assert(nextTask != gKernel.schedulerTask);
            co_await ResumeTaskOp(nextTask);
            assert(gKernel.currentTask.isValid());
            continue;
        }
        std::print(">> SchedulerTask({}): ready count: 0; zombie count: {}\n", (void*)&gKernel.currentTask.promise(), gKernel.zombieCount);
        
        // Zombie bashing
        while (gKernel.zombieCount > 0) {
            std::print(">> SchedulerTask({}): about to delete a zombie (remaining zombies: {})\n", (void*)&gKernel.currentTask.promise(), gKernel.zombieCount); 
            DebugTaskCount();

            Link* zombieNode = DequeueLink(&gKernel.zombieList);
            TaskPromise& zombiePromise = *waitListNodeToTaskPromise(zombieNode);
            assert(zombiePromise.state == TaskState::ZOMBIE);
            
            // Remove from zombie list
            --gKernel.zombieCount;
            DetachLink(&zombiePromise.waitNode);

            // Remove from task list
            DetachLink(&zombiePromise.taskList);
            --gKernel.taskCount;

            // Delete
            zombiePromise.state = TaskState::DELETING;
            TaskHdl zombieTaskHdl = TaskHdl::from_promise(zombiePromise);
            zombieTaskHdl.destroy();

            std::print(">> SchedulerTask({}): did delete a zombie\n", (void*)&gKernel.currentTask.promise()); 
            DebugTaskCount();
        }
        
        if (gKernel.readyCount == 0) break;
    }
    std::print(">> SchedulerTask({}): scheduler task terminated\n", (void*)&gKernel.currentTask.promise());
    co_await TerminateSchedulerOp {};
    
    assert(false); // Unreachale
    co_return;
}


inline void initKernel() noexcept {
    using namespace internal_ak;
    gKernel.taskCount = 0;
    gKernel.readyCount = 0;
    gKernel.waitingCount = 0;
    gKernel.ioWaitingCount = 0;
    gKernel.zombieCount = 0;
    gKernel.interrupted = 0;
    gKernel.currentTask.clear();
    gKernel.schedulerTask.clear();
    InitLink(&gKernel.zombieList);
    InitLink(&gKernel.readyList);
    InitLink(&gKernel.taskList);
    std::print("Kernel::init(): initialized\n");
}

inline void finiKernel() noexcept {
    // TODO: add checks to ensure all tasks are finalized

    std::print("Kernel::fini(): finalized\n");
}

inline TaskState SchedulerTask::state() const noexcept {
    return hdl.promise().state;
}

// -----------------------------------------------------------------------------
// TaskPromise
// -----------------------------------------------------------------------------

inline void* TaskPromise::operator new(std::size_t n) noexcept {
    std::print("TaskPromise: About to allocate a Task of size: {}\n", n);
    void* mem = std::malloc(n);
    if (!mem)
        return nullptr;
    unsigned long long frameHead = (unsigned long long)mem;
    unsigned long long frameTail = frameHead + n;
    unsigned long long taskAddr  = frameTail - sizeof(TaskPromise) - 8;
    std::print("TaskPromise: Did allocate a Task({}) of size: {}\n", (void*)taskAddr, n); 
    return mem;
}

inline void TaskPromise::operator delete(void* ptr, std::size_t sz) {
    unsigned long long frameHead = (unsigned long long)ptr;
    unsigned long long frameTail = frameHead + sz;
    unsigned long long taskAddr  = frameTail - sizeof(TaskPromise) - 8;
    std::print("TaskPromise: About to free Task({}) of size: {}\n", (void*)taskAddr, sz);  
    
    std::free(ptr);
    std::print("TaskPromise: Did free Task({}) of size: {}\n", (void*)taskAddr, sz);   
}

inline TaskPromise::TaskPromise() {
    using namespace internal_ak;

    InitLink(&taskList);
    InitLink(&waitNode);    
    InitLink(&waitTaskNode);
    state = TaskState::CREATED;
    std::print("TaskPromise::TaskPromise(): Task({}) initialized\n", (void*)this); 
    
    // Check post-conditions
    assert(IsLinkDetached(&taskList));
    assert(IsLinkDetached(&waitNode));
    assert(state == TaskState::CREATED);
    CheckInvariants();
}

inline TaskPromise::~TaskPromise() {
    using namespace internal_ak;
    std::print("TaskPromise::TaskPromise(): about to finalize Task({})\n", (void*)this); 
    assert(state == TaskState::DELETING);
    assert(IsLinkDetached(&taskList));
    assert(IsLinkDetached(&waitNode));

    std::print("TaskPromise::TaskPromise(): did finalize Task({})\n", (void*)this); 
    DebugTaskCount();
    CheckInvariants();
}

inline TaskHdl TaskPromise::get_return_object() noexcept { 
    std::print("TaskPromise::TaskPromise(): Task({}) returning TaskHdl\n", (void*)this);
    return TaskHdl::from_promise(*this);
}

inline void TaskPromise::return_void() noexcept {
    using namespace internal_ak;
    std::print("TaskPromise::TaskPromise(): Task({}) returned void\n", (void*)this);
    CheckInvariants();
    
    // Wake up all tasks waiting for this task
    if (IsLinkDetached(&waitTaskNode)) {
        std::print("TaskPromise::TaskPromise(): Task({}) there are no waiting tasks\n", (void*)this);
        return;
    }

    std::print("TaskPromise::TaskPromise(): Task({}) waking up waiting tasks\n", (void*)this);
    do {
        Link* next = DequeueLink(&waitTaskNode);
        TaskPromise& nextPromise = *waitListNodeToTaskPromise(next);

        std::print("TaskPromise::TaskPromise(): Task({}) about to wake up Task({})\n", (void*)this, (void*)&nextPromise); 
        DebugTaskCount();

        assert(nextPromise.state == TaskState::WAITING);
        --gKernel.waitingCount;
        nextPromise.state = TaskState::READY;
        EnqueueLink(&gKernel.readyList, &nextPromise.waitNode);
        ++gKernel.readyCount;
        
        std::print("TaskPromise::TaskPromise(): Task({}) did wake up Task({})\n", (void*)this, (void*)&nextPromise);
        DebugTaskCount();

    } while (!IsLinkDetached(&waitTaskNode));
}

// TaskPromise::InitialSuspend -------------------------------------------------

inline void TaskPromise::InitialSuspend::await_suspend(TaskHdl hdl) const noexcept {
    using namespace internal_ak;
    TaskPromise& promise = hdl.promise();

    std::print("TaskPromise::InitialSuspend::await_suspend(TaskHdl hdl): initial suspend started({})\n", (void*)&promise);  
    
    // Check initial preconditions
    assert(promise.state == TaskState::CREATED);
    assert(IsLinkDetached(&promise.waitNode));
    CheckInvariants();

    // Add task to the kernel
    ++gKernel.taskCount;    
    EnqueueLink(&gKernel.taskList, &promise.taskList);

    ++gKernel.readyCount;
    EnqueueLink(&gKernel.readyList, &promise.waitNode);
    promise.state = TaskState::READY;

    // Check post-conditions
    assert(promise.state == TaskState::READY);
    assert(!IsLinkDetached(&promise.waitNode));
    CheckInvariants();

    std::print("TaskPromise::InitialSuspend::await_suspend(TaskHdl hdl): newly spawned Task({}) is {}\n", (void*)&promise, (int)promise.state); 
    internal_ak::DebugTaskCount();
}


// TaskPromise::FinalSuspend -------------------------------------------------

inline TaskHdl TaskPromise::FinalSuspend::await_suspend(TaskHdl currentTaskHdl) const noexcept {
    // Check preconditions
    TaskPromise& currentPromise = currentTaskHdl.promise();                            
    assert(gKernel.currentTask == currentTaskHdl);
    assert(currentPromise.state == TaskState::RUNNING);
    assert(IsLinkDetached(&currentPromise.waitNode));
    internal_ak::CheckInvariants();
    
    // Move the current task from RUNNING to ZOMBIE
    currentPromise.state = TaskState::ZOMBIE;
    ++gKernel.zombieCount;
    EnqueueLink(&gKernel.zombieList, &currentPromise.waitNode);
    gKernel.currentTask.clear();
    internal_ak::CheckInvariants();

    // Move the SchedulerTask from READY to RUNNING
    TaskPromise& schedulerPromise = gKernel.schedulerTask.promise();
    schedulerPromise.state = TaskState::RUNNING;    
    DetachLink(&schedulerPromise.waitNode); // remove from ready list
    --gKernel.readyCount;
    gKernel.currentTask = gKernel.schedulerTask;
    internal_ak::CheckInvariants();
    
    return gKernel.schedulerTask;
}

// -----------------------------------------------------------------------------
// Debug utilities
// -----------------------------------------------------------------------------
namespace internal_ak {

    inline void DebugTaskCount() noexcept {
        int running_count = gKernel.currentTask != TaskHdl() ? 1 : 0;
        std::print("----------------:--------\n"); 
        std::print("Task       count: {}\n", gKernel.taskCount);
        std::print("----------------:--------\n"); 
        std::print("Running    count: {}\n", running_count); 
        std::print("Ready      count: {}\n", gKernel.readyCount);
        std::print("Waiting    count: {}\n", gKernel.waitingCount);
        std::print("IO waiting count: {}\n", gKernel.ioWaitingCount);
        std::print("Zombie     count: {}\n", gKernel.zombieCount);
        std::print("----------------:--------\n"); 
    }

    inline static void DoCheckTaskCountInvariant() noexcept {
        int running_count = gKernel.currentTask != TaskHdl() ? 1 : 0;
        bool condition = gKernel.taskCount == running_count + gKernel.readyCount + gKernel.waitingCount + gKernel.ioWaitingCount + gKernel.zombieCount; // -1 for the running task
        if (!condition) {
            DebugTaskCount();
            std::abort();
        }
    }

    inline void CheckTaskCountInvariant() noexcept {
        if constexpr (DEFINED_DEBUG) {
            DoCheckTaskCountInvariant();
        }
    }

    inline void CheckInvariants() noexcept {
        if constexpr (DEFINED_DEBUG) {
            // check the Task invariants
            DoCheckTaskCountInvariant();

            // Ensure that the current Task is valid
            // if (gKernel.currentTask.isValid()) std::abort();
        }    
    }

}

struct Kernel gKernel;



