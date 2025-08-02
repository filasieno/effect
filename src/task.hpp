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
struct DefineTask;
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
using TaskFn = std::function<DefineTask(Args...)>;

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


/// \defgroup Task Task API
/// \brief Task API defines the API for creating and managing tasks.



// -----------------------------------------------------------------------------
// Utils
// -----------------------------------------------------------------------------
namespace ak_internal {

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


struct Kernel {
    int   taskCount;
    int   readyCount;      // number of live tasks
    int   waitingCount;    // task waiting for execution (On Internal Critical sections)
    int   ioWaitingCount;  // task waiting for IO URing
    int   zombieCount;     // dead tasks

    int     interrupted;
    TaskHdl currentTaskHdl;
    TaskHdl schedulerTaskHdl;

    ak_internal::Link zombieList;
    ak_internal::Link readyList;
    ak_internal::Link taskList;        // global task list

    KernelBootTaskHdl kernelTask;
};

// -----------------------------------------------------------------------------
// Task Promise
// -----------------------------------------------------------------------------

inline void ClearTask(TaskHdl*hdl) noexcept {
    *hdl = TaskHdl{};
}

namespace ak_internal {

    void DebugTaskCount() noexcept;
    void CheckInvariants() noexcept;

    struct InitialSuspendTaskOp {
        constexpr bool await_ready() const noexcept { return false; }
        void           await_suspend(TaskHdl hdl) const noexcept;
        constexpr void await_resume() const noexcept {}
    };

    struct FinalSuspendTaskOp {
        constexpr bool await_ready() const noexcept { return false; }
        TaskHdl        await_suspend(TaskHdl hdl) const noexcept;
        constexpr void await_resume() const noexcept { assert(false); }
    };

}

struct TaskPromise {

    void* operator new(std::size_t n) noexcept {
        void* mem = std::malloc(n);
        if (!mem) return nullptr;
        return mem;
    }

    void  operator delete(void* ptr, std::size_t sz) {
        (void)sz;
        std::free(ptr);
    }

    TaskPromise() {
        using namespace ak_internal;

        InitLink(&taskListLink);
        InitLink(&waitLink);
        InitLink(&awaitingTerminationList);
        state = TaskState::CREATED;

        // Check post-conditions
        assert(IsLinkDetached(&taskListLink));
        assert(IsLinkDetached(&waitLink));
        assert(state == TaskState::CREATED);
        CheckInvariants();
    }

    ~TaskPromise() {
        using namespace ak_internal;
        assert(state == TaskState::DELETING);
        assert(IsLinkDetached(&taskListLink));
        assert(IsLinkDetached(&waitLink));
        DebugTaskCount();
        CheckInvariants();
    }

    TaskHdl        get_return_object() noexcept    { return TaskHdl::from_promise(*this);}
    constexpr auto initial_suspend() noexcept      { return ak_internal::InitialSuspendTaskOp{}; }
    constexpr auto final_suspend() noexcept        { return ak_internal::FinalSuspendTaskOp{}; }
    void           return_void() noexcept;
    void           unhandled_exception() noexcept  { assert(false); }

    TaskState         state;
    ak_internal::Link waitLink;                // Used to enqueue tasks waiting for Critical Section
    ak_internal::Link taskListLink;            // Global Task list
    ak_internal::Link awaitingTerminationList; // The list of all tasks waiting for this task
};



namespace ak_internal {

    struct ResumeTaskOp {

        explicit ResumeTaskOp(TaskHdl hdl) : hdl(hdl) {};

        constexpr bool await_ready() const noexcept { return hdl.done();}
        TaskHdl        await_suspend(TaskHdl currentTaskHdl) const noexcept;
        constexpr void await_resume() const noexcept {}

        TaskHdl hdl;
    };

    struct JoinTaskOp {
        explicit JoinTaskOp(TaskHdl hdl) : hdl(hdl) {};

        constexpr bool await_ready() const noexcept { return hdl.done(); }
        constexpr TaskHdl await_suspend(TaskHdl currentTaskHdl) const noexcept;
        constexpr void await_resume() const noexcept {}

        TaskHdl hdl;
    };

};

/// \brief Marks a Task coroutine function
struct DefineTask {
    using promise_type = TaskPromise;

    DefineTask(const TaskHdl& hdl) : hdl(hdl) {}
    operator TaskHdl() const noexcept { return hdl; }

    TaskHdl hdl;
};

inline bool IsTaskHdlValid(TaskHdl hdl) {
    return hdl.address() != nullptr;
}

inline TaskPromise* GetTaskPromise(TaskHdl hdl) {
    return &hdl.promise();
}

// -----------------------------------------------------------------------------
// Kernel s
// -----------------------------------------------------------------------------


extern struct Kernel gKernel;

// -----------------------------------------------------------------------------
// Kernel API
// -----------------------------------------------------------------------------

int RunMain(std::function<DefineTask()> userMainTask) noexcept;

// -----------------------------------------------------------------------------
// Debug & invariant Utilities
// -----------------------------------------------------------------------------

namespace ak_internal {
    void CheckInvariants() noexcept;
    void DebugTaskCount() noexcept;
}

// -----------------------------------------------------------------------------
// Task Wrappers
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Utilities
// -----------------------------------------------------------------------------
namespace ak_internal {
    static TaskPromise* waitListNodeToTaskPromise(const Link* node) noexcept {
        unsigned long long promise_off = ((unsigned long long)node) - offsetof(TaskPromise, waitLink);
        return (TaskPromise*)promise_off;
    }
}

// -----------------------------------------------------------------------------
// Effects
// -----------------------------------------------------------------------------

namespace ak_internal {

    struct SuspendOp {
        constexpr bool await_ready() const noexcept { return false; }
        TaskHdl        await_suspend(TaskHdl hdl) const noexcept;
        constexpr void await_resume() const noexcept {};
    };

    struct GetCurrentTaskOp {
        constexpr bool    await_ready() const noexcept        { return false; }
        constexpr TaskHdl await_suspend(TaskHdl hdl) noexcept { this->hdl = hdl; return hdl; }
        constexpr TaskHdl await_resume() const noexcept       { return hdl; }

        TaskHdl hdl;
    };
}

inline auto GetCurrentTask() noexcept {
    return ak_internal::GetCurrentTaskOp{};
}

inline constexpr auto SuspendTask() noexcept { return ak_internal::SuspendOp{}; }

/// \brief Suspends the current Task until the target Task completes.
/// \param hdl a handle to the target Task.
/// \return an awaitable object that returns void
/// \ingroup Task
inline auto JoinTask(TaskHdl hdl) noexcept {
	return ak_internal::JoinTaskOp{hdl};
}

/// \brief Alias for AkJoinTask
/// \param hdl a handle to the target Task.
/// \return an awaitable object that returns void
/// \ingroup Task
inline auto operator co_await(TaskHdl hdl) noexcept {
	return JoinTask(hdl);
}

/// \brief Resturns the current TaskState.
/// \param hdl a handle to the target Task.
/// \return the current TaskState
/// \ingroup Task
inline TaskState GetTaskState(TaskHdl hdl) noexcept {
	return hdl.promise().state;
}

/// \brief Returns true if the target Task is done.
/// \param hdl a handle to the target Task
/// \return true if the target Task is done
/// \ingroup Task
inline bool IsTaskDone(TaskHdl hdl) noexcept {
	return hdl.done();
}

/// \brief Resumes a Task that is in TaskState::READY
/// \param hdl a handle to the target Task
/// \return true if the target Task is done
/// \ingroup Task
inline auto ResumeTask(TaskHdl hdl) noexcept {
	return ak_internal::ResumeTaskOp{hdl};
}

// Task::AwaitTaskEffect
// ----------------------------------------------------------------------------------------------------------------

inline constexpr TaskHdl ak_internal::JoinTaskOp::await_suspend(TaskHdl currentTaskHdl) const noexcept
{
    using namespace ak_internal;

    TaskPromise& currentTaskPromise = currentTaskHdl.promise();
    TaskPromise& hdlTaskPromise = hdl.promise();

    // Check preconditions

    assert(currentTaskPromise.state == TaskState::RUNNING);
    assert(IsLinkDetached(&currentTaskPromise.waitLink));
    assert(gKernel.currentTaskHdl == currentTaskHdl);
    CheckInvariants();

    // Move the current task from RUNNINIG to WAITING

    currentTaskPromise.state = TaskState::WAITING;
    ++gKernel.waitingCount;
    EnqueueLink(&hdlTaskPromise.awaitingTerminationList, &currentTaskPromise.waitLink);
    ClearTask(&gKernel.currentTaskHdl);
    CheckInvariants();
    DebugTaskCount();
    // if (hdlTaskPromise.state == TaskState::READY) {
    //     // Just move to READY
    //     return hdlTaskPromise;
    // }

    // Move the target task from READY to RUNNING
    TaskPromise& schedulerPromise = gKernel.schedulerTaskHdl.promise();
    assert(schedulerPromise.state == TaskState::READY);
    schedulerPromise.state = TaskState::RUNNING;
    DetachLink(&schedulerPromise.waitLink);
    --gKernel.readyCount;
    gKernel.currentTaskHdl = gKernel.schedulerTaskHdl;
    CheckInvariants();
    DebugTaskCount();

    return gKernel.schedulerTaskHdl;
}

inline TaskHdl ak_internal::SuspendOp::await_suspend(TaskHdl currentTask) const noexcept {
    using namespace ak_internal;
    assert(gKernel.currentTaskHdl);

    TaskPromise& currentPromise = currentTask.promise();

    if constexpr (DEFINED_DEBUG) {
        assert(gKernel.currentTaskHdl == currentTask);
        assert(currentPromise.state == TaskState::RUNNING);
        assert(IsLinkDetached(&currentPromise.waitLink));
        CheckInvariants();
    }

    // Move the current task from RUNNINIG to READY
    currentPromise.state = TaskState::READY;
    ++gKernel.readyCount;
    EnqueueLink(&gKernel.readyList, &currentPromise.waitLink);
    ClearTask(&gKernel.currentTaskHdl);
    CheckInvariants();

    // Resume the SchedulerTask
    TaskPromise& schedulerPromise = gKernel.schedulerTaskHdl.promise();
    schedulerPromise.state = TaskState::RUNNING;
    DetachLink(&schedulerPromise.waitLink); // remove from ready list
    --gKernel.readyCount;
    gKernel.currentTaskHdl = gKernel.schedulerTaskHdl;
    CheckInvariants();

    assert(gKernel.currentTaskHdl);
    return TaskHdl::from_promise(schedulerPromise);
}

inline TaskHdl ak_internal::ResumeTaskOp::await_suspend(TaskHdl currentTaskHdl) const noexcept {
    using namespace ak_internal;
    assert(gKernel.currentTaskHdl == currentTaskHdl);

    // Check the current Task
    TaskPromise& currentPromise = gKernel.currentTaskHdl.promise();
    assert(IsLinkDetached(&currentPromise.waitLink));
    assert(currentPromise.state == TaskState::RUNNING);
    CheckInvariants();

    // Suspend the current Task
    currentPromise.state = TaskState::READY;
    ++gKernel.readyCount;
    EnqueueLink(&gKernel.readyList, &currentPromise.waitLink);
    gKernel.currentTaskHdl = TaskHdl();
    CheckInvariants();

    // Move the target task from READY to RUNNING
    TaskPromise& promise = hdl.promise();
    promise.state = TaskState::RUNNING;
    DetachLink(&promise.waitLink);
    --gKernel.readyCount;
    gKernel.currentTaskHdl = hdl;
    CheckInvariants();

    assert(gKernel.currentTaskHdl);
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
        using namespace ak_internal;

        (void)currentTaskHdl;
        TaskPromise& schedulerPromise = schedulerHdl.promise();

        // Check expected state post scheduler construction

        assert(gKernel.taskCount == 1);
        assert(gKernel.readyCount == 1);
        assert(schedulerPromise.state == TaskState::READY);
        assert(!IsLinkDetached(&schedulerPromise.waitLink));
        assert(gKernel.currentTaskHdl == TaskHdl());

        // Setup SchedulerTask for execution (from READY -> RUNNING)
        gKernel.currentTaskHdl = schedulerHdl;
        schedulerPromise.state = TaskState::RUNNING;
        DetachLink(&schedulerPromise.waitLink);
        --gKernel.readyCount;

        // Check expected state post task system bootstrap
        CheckInvariants();
        return schedulerHdl;
    }

    void await_resume() const noexcept {}

    TaskHdl schedulerHdl;
};

struct TerminateSchedulerOp {
    constexpr bool await_ready() const noexcept { return false; }
    KernelBootTaskHdl await_suspend(TaskHdl hdl) const noexcept {
        using namespace ak_internal;

        assert(gKernel.currentTaskHdl == gKernel.schedulerTaskHdl);
        assert(gKernel.currentTaskHdl == hdl);

        TaskPromise& schedulerPromise = gKernel.schedulerTaskHdl.promise();
        assert(schedulerPromise.state == TaskState::RUNNING);
        assert(IsLinkDetached(&schedulerPromise.waitLink));

        schedulerPromise.state = TaskState::ZOMBIE;
        ClearTask(&gKernel.currentTaskHdl);
        EnqueueLink(&gKernel.zombieList, &schedulerPromise.waitLink);
        ++gKernel.zombieCount;

        return gKernel.kernelTask;
    }
    constexpr void await_resume() const noexcept {}
};

RunSchedulerTaskOp RunSchedulerTask(TaskHdl hdl) noexcept {
    return RunSchedulerTaskOp{hdl};
}

void DestroySchedulerTask(TaskHdl hdl) noexcept {
    TaskPromise* promise = &hdl.promise();

    // Remove from Task list
    DetachLink(&promise->taskListLink);
    --gKernel.taskCount;

    // Remove from Zombie List
    DetachLink(&promise->waitLink);
    --gKernel.zombieCount;

    promise->state = TaskState::DELETING; //TODO: double check
    hdl.destroy();
}

struct KernelBootPromise {
    KernelBootPromise(std::function<DefineTask()> userMainTask) : userMainTask(userMainTask) {}

    KernelBootTaskHdl get_return_object() noexcept   { return KernelBootTaskHdl::from_promise(*this); }
    constexpr auto    initial_suspend() noexcept     { return std::suspend_always {}; }
    constexpr auto    final_suspend() noexcept       { return std::suspend_never  {}; }
    constexpr void    return_void() noexcept         {}
    constexpr void    unhandled_exception() noexcept { assert(false); }

    std::function<DefineTask()> userMainTask;
};

struct KernelBootTask {
    using promise_type = KernelBootPromise;

    KernelBootTask(KernelBootTaskHdl hdl) noexcept : hdl(hdl) {}

    KernelBootTaskHdl hdl;
};

static KernelBootTask mainKernelTask(std::function<DefineTask()> userMainTask) noexcept;
static DefineTask     schedulerTask(std::function<DefineTask()> userMainTask) noexcept;

inline int RunMain(std::function<DefineTask()> userMainTask) noexcept {
    using namespace ak_internal;
    std::setbuf(stdout, nullptr);
    std::setbuf(stderr, nullptr);

    initKernel();

    KernelBootTask kernelBootTask = mainKernelTask(userMainTask);
    gKernel.kernelTask = kernelBootTask.hdl;
    kernelBootTask.hdl.resume();

    finiKernel();

    return 0;
}

inline KernelBootTask mainKernelTask(std::function<DefineTask()> userMainTask) noexcept {
    using namespace ak_internal;

    TaskHdl schedulerHdl = schedulerTask(userMainTask);
    gKernel.schedulerTaskHdl = schedulerHdl;

    co_await RunSchedulerTask(schedulerHdl);
    DestroySchedulerTask(schedulerHdl);
    DebugTaskCount();

    co_return;
}

static int started = 0;

inline DefineTask schedulerTask(std::function<DefineTask()> userMainTask) noexcept {
    using namespace ak_internal;

    ++started;
    std::fflush(stdout);
    assert(started == 1);

    TaskHdl mainTask = userMainTask();
    assert(!mainTask.done());
    assert(GetTaskState(mainTask) == TaskState::READY);

    DebugTaskCount();

    while (true) {

        // If we have a ready task, resume it
        if (gKernel.readyCount > 0) {
            Link* nextNode = gKernel.readyList.prev;
            TaskPromise* nextPromise = waitListNodeToTaskPromise(nextNode);
            TaskHdl nextTask = TaskHdl::from_promise(*nextPromise);
            assert(nextTask != gKernel.schedulerTaskHdl);
            co_await ResumeTaskOp(nextTask);
            assert(gKernel.currentTaskHdl);
            continue;
        }

        // Zombie bashing
        while (gKernel.zombieCount > 0) {
            DebugTaskCount();

            Link* zombieNode = DequeueLink(&gKernel.zombieList);
            TaskPromise& zombiePromise = *waitListNodeToTaskPromise(zombieNode);
            assert(zombiePromise.state == TaskState::ZOMBIE);

            // Remove from zombie list
            --gKernel.zombieCount;
            DetachLink(&zombiePromise.waitLink);

            // Remove from task list
            DetachLink(&zombiePromise.taskListLink);
            --gKernel.taskCount;

            // Delete
            zombiePromise.state = TaskState::DELETING;
            TaskHdl zombieTaskHdl = TaskHdl::from_promise(zombiePromise);
            zombieTaskHdl.destroy();

            DebugTaskCount();
        }

        if (gKernel.readyCount == 0) break;
    }
    co_await TerminateSchedulerOp {};

    assert(false); // Unreachale
    co_return;
}


inline void initKernel() noexcept {
    using namespace ak_internal;
    gKernel.taskCount = 0;
    gKernel.readyCount = 0;
    gKernel.waitingCount = 0;
    gKernel.ioWaitingCount = 0;
    gKernel.zombieCount = 0;
    gKernel.interrupted = 0;
    ClearTask(&gKernel.currentTaskHdl);
    ClearTask(&gKernel.schedulerTaskHdl);
    InitLink(&gKernel.zombieList);
    InitLink(&gKernel.readyList);
    InitLink(&gKernel.taskList);
}

inline void finiKernel() noexcept { }

// -----------------------------------------------------------------------------
// TaskPromise
// -----------------------------------------------------------------------------

inline void TaskPromise::return_void() noexcept {
    using namespace ak_internal;
    CheckInvariants();

    // Wake up all tasks waiting for this task
    if (IsLinkDetached(&awaitingTerminationList)) {
        return;
    }

    do {
        Link* next = DequeueLink(&awaitingTerminationList);
        TaskPromise& nextPromise = *waitListNodeToTaskPromise(next);
        DebugTaskCount();
        assert(nextPromise.state == TaskState::WAITING);
        --gKernel.waitingCount;
        nextPromise.state = TaskState::READY;
        EnqueueLink(&gKernel.readyList, &nextPromise.waitLink);
        ++gKernel.readyCount;
        DebugTaskCount();

    } while (!IsLinkDetached(&awaitingTerminationList));
}

// TaskPromise::InitialSuspend -------------------------------------------------

inline void ak_internal::InitialSuspendTaskOp::await_suspend(TaskHdl hdl) const noexcept {
    using namespace ak_internal;
    TaskPromise& promise = hdl.promise();

    // Check initial preconditions
    assert(promise.state == TaskState::CREATED);
    assert(IsLinkDetached(&promise.waitLink));
    CheckInvariants();

    // Add task to the kernel
    ++gKernel.taskCount;
    EnqueueLink(&gKernel.taskList, &promise.taskListLink);

    ++gKernel.readyCount;
    EnqueueLink(&gKernel.readyList, &promise.waitLink);
    promise.state = TaskState::READY;

    // Check post-conditions
    assert(promise.state == TaskState::READY);
    assert(!IsLinkDetached(&promise.waitLink));
    CheckInvariants();
    ak_internal::DebugTaskCount();
}


// TaskPromise::FinalSuspend -------------------------------------------------

inline TaskHdl ak_internal::FinalSuspendTaskOp::await_suspend(TaskHdl currentTaskHdl) const noexcept {
    using namespace ak_internal;
    // Check preconditions
    TaskPromise& currentPromise = currentTaskHdl.promise();
    assert(gKernel.currentTaskHdl == currentTaskHdl);
    assert(currentPromise.state == TaskState::RUNNING);
    assert(IsLinkDetached(&currentPromise.waitLink));
    ak_internal::CheckInvariants();

    // Move the current task from RUNNING to ZOMBIE
    currentPromise.state = TaskState::ZOMBIE;
    ++gKernel.zombieCount;
    EnqueueLink(&gKernel.zombieList, &currentPromise.waitLink);
    ClearTask(&gKernel.currentTaskHdl);
    ak_internal::CheckInvariants();

    // Move the SchedulerTask from READY to RUNNING
    TaskPromise& schedulerPromise = gKernel.schedulerTaskHdl.promise();
    schedulerPromise.state = TaskState::RUNNING;
    DetachLink(&schedulerPromise.waitLink); // remove from ready list
    --gKernel.readyCount;
    gKernel.currentTaskHdl = gKernel.schedulerTaskHdl;
    ak_internal::CheckInvariants();

    return gKernel.schedulerTaskHdl;
}

// -----------------------------------------------------------------------------
// Debug utilities
// -----------------------------------------------------------------------------
namespace ak_internal {

    inline void DebugTaskCount() noexcept {
        int running_count = gKernel.currentTaskHdl != TaskHdl() ? 1 : 0;
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
        int running_count = gKernel.currentTaskHdl != TaskHdl() ? 1 : 0;
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


struct Condition {


    struct WaitOp {

        explicit WaitOp(Condition& condition) : condition(condition) {}

        constexpr bool await_ready() const noexcept {
            return IsTaskDone(condition.lockingTask);
        }

        constexpr TaskHdl await_suspend(TaskHdl currentTaskHdl) const noexcept {
            using namespace ak_internal;

            TaskPromise& currentTaskPromise = gKernel.currentTaskHdl.promise();

            assert(gKernel.currentTaskHdl == currentTaskHdl);


            // Check the current Task

            assert(IsLinkDetached(&currentTaskPromise.waitLink));
            assert(currentTaskPromise.state == TaskState::RUNNING);
            ak_internal::CheckInvariants();

            // Move the current task from READY to WAITING into the condition
            currentTaskPromise.state = TaskState::WAITING;
            ++gKernel.waitingCount;
            EnqueueLink(&condition.waitNode, &currentTaskPromise.waitLink);
            ClearTask(&gKernel.currentTaskHdl);
            ak_internal::CheckInvariants();

            // Move the target task from READY to RUNNING
            TaskPromise& schedulerPromise = gKernel.schedulerTaskHdl.promise();
            schedulerPromise.state = TaskState::RUNNING;
            DetachLink(&schedulerPromise.waitLink); // remove from ready list
            --gKernel.readyCount;
            gKernel.currentTaskHdl = gKernel.schedulerTaskHdl;
            ak_internal::CheckInvariants();
            return gKernel.schedulerTaskHdl;
        }

        constexpr void await_resume() const noexcept {}

        Condition& condition;
    };

    Condition(bool signaled = false) : signaled(signaled) {
        using namespace ak_internal;
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
        using namespace ak_internal;
        this->signaled = signaled;
        ClearTask(&lockingTask);
        InitLink(&waitNode);
    }

    // g_kernel.current_task_hdl = TaskHdl();
    // g_kernel.current_task.clear();
    bool              signaled = false;
    TaskHdl           lockingTask;
    ak_internal::Link waitNode;
};

struct Kernel gKernel;



