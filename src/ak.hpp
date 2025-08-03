#pragma once

#include <cassert>
#include <coroutine>
#include <print>
#include <functional>

namespace ak_internal {
#ifdef NDEBUG
    constexpr bool IS_DEBUG_MODE = false;
#else
    constexpr bool IS_DEBUG_MODE = true;
#endif
}

// -----------------------------------------------------------------------------

/// \defgroup Task Task API
/// \brief Task API defines the API for creating and managing tasks.

/// \defgroup Kernel API
/// \brief Kernel API defines system level APIs.

// -----------------------------------------------------------------------------

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

struct DefineTask;

struct TaskContext;

/// \brief Coroutine handle for a Task
/// \ingroup Task
using TaskHdl = std::coroutine_handle<TaskContext>;

/// \brief Defines a Task function
/// \ingroup Task
template <typename... Args>
using TaskFn = std::function<DefineTask(Args...)>;

namespace ak_internal {

    struct KernelTaskPromise;
    using KernelTaskHdl = std::coroutine_handle<KernelTaskPromise>;

    struct Link {
        Link* next;
        Link* prev;
    };

    inline void InitLink(Link* link) {
        link->next = link;
        link->prev = link;
    }

    inline bool IsLinkDetached(const Link* link) {
        assert(link != nullptr);
        assert(link->next != nullptr);
        assert(link->prev != nullptr);
        return link->next == link && link->prev == link;
    }

    inline void DetachLink(Link* link) {
        assert(link != nullptr);
        assert(link->next != nullptr);
        assert(link->prev != nullptr);
        if (IsLinkDetached(link)) return;
        link->next->prev = link->prev;
        link->prev->next = link->next;
        link->next = link;
        link->prev = link;
    }

    inline void EnqueueLink(Link* queue, Link* link) {
        assert(link != nullptr);
        assert(queue->next != nullptr);
        assert(queue->prev != nullptr);
        assert(IsLinkDetached(link));

        link->next = queue->next;
        link->prev = queue;

        link->next->prev = link;
        queue->next = link;
    }

    inline Link* DequeueLink(Link* queue) {
        assert(queue->next != nullptr);
        assert(queue->prev != nullptr);
        if (IsLinkDetached(queue)) return nullptr;
        Link* target = queue->prev;
        DetachLink(target);
        return target;
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

        Link zombieList;
        Link readyList;
        Link taskList;        // global task list

        KernelTaskHdl kernelTask;
    };

    extern struct Kernel gKernel;

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

    void CheckInvariants() noexcept;
    
    void DebugTaskCount() noexcept;
    
}

/// \brief Define a context.
/// \ingroup Task
struct TaskContext {
    using Link = ak_internal::Link;

    void* operator new(std::size_t n) noexcept {
        void* mem = std::malloc(n);
        if (!mem) return nullptr;
        return mem;
    }

    void  operator delete(void* ptr, std::size_t sz) {
        (void)sz;
        std::free(ptr);
    }

    TaskContext() {
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

    ~TaskContext() {
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

    TaskState state;
    Link      waitLink;                // Used to enqueue tasks waiting for Critical Section
    Link      taskListLink;            // Global Task list
    Link      awaitingTerminationList; // The list of all tasks waiting for this task
};

/// \brief Marks a Task coroutine function
struct DefineTask {
    using promise_type = TaskContext;

    DefineTask(const TaskHdl& hdl) : hdl(hdl) {}
    operator TaskHdl() const noexcept { return hdl; }

    TaskHdl hdl;
};

/// \brief Clears the target TaskHdl
/// \param hdl the handle to be cleared
/// \ingroup Task
inline void ClearTaskHdl(TaskHdl* hdl) noexcept {
    *hdl = TaskHdl{};
}

/// \brief Checks is the a TaskHdl is valid
/// \param hdl the handle to be cleared
/// \ingroup Task
inline bool IsTaskHdlValid(TaskHdl hdl) {
    return hdl.address() != nullptr;
}

/// \brief Returns the TaskPromise associated with the target TaskHdl
/// @param hdl 
/// @return the TaskPromise associated with the target TaskHdl
/// \ingroup Task
inline TaskContext* GetTaskTaskContext(TaskHdl hdl) {
    return &hdl.promise();
}

/// \brief Get the current Task
/// \return [Async] TaskHdl
/// \ingroup Task
inline constexpr auto GetCurrentTask() noexcept {
    return ak_internal::GetCurrentTaskOp{};
}

/// \brief Suspends the current Task and resumes the Scheduler.
/// \return [Async] void
/// \ingroup Task
inline constexpr auto SuspendTask() noexcept { return ak_internal::SuspendOp{}; }

/// \brief Suspends the current Task until the target Task completes.
/// \param hdl a handle to the target Task.
/// \return [Async] void
/// \ingroup Task
inline auto JoinTask(TaskHdl hdl) noexcept {
	return ak_internal::JoinTaskOp{hdl};
}

/// \brief Alias for AkJoinTask
/// \param hdl a handle to the target Task.
/// \return [Async] void
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
/// \return `true` if the target Task is done
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

namespace ak_internal 
{
    inline static TaskContext* waitListNodeToTaskPromise(const Link* link) noexcept {
        unsigned long long promise_off = ((unsigned long long)link) - offsetof(TaskContext, waitLink);
        return (TaskContext*)promise_off;
    }

    auto RunSchedulerTask() noexcept {
        struct RunSchedulerTaskOp {

            constexpr bool await_ready() const noexcept  { return false; }
            constexpr void await_resume() const noexcept {}  

            TaskHdl await_suspend(KernelTaskHdl currentTaskHdl) const noexcept {
                using namespace ak_internal;

                (void)currentTaskHdl;
                TaskContext& schedulerPromise = gKernel.schedulerTaskHdl.promise();

                // Check expected state post scheduler construction

                assert(gKernel.taskCount == 1);
                assert(gKernel.readyCount == 1);
                assert(schedulerPromise.state == TaskState::READY);
                assert(!IsLinkDetached(&schedulerPromise.waitLink));
                assert(gKernel.currentTaskHdl == TaskHdl());

                // Setup SchedulerTask for execution (from READY -> RUNNING)
                gKernel.currentTaskHdl = gKernel.schedulerTaskHdl;
                schedulerPromise.state = TaskState::RUNNING;
                DetachLink(&schedulerPromise.waitLink);
                --gKernel.readyCount;

                // Check expected state post task system bootstrap
                CheckInvariants();
                return gKernel.schedulerTaskHdl;
            }        
        };

        return RunSchedulerTaskOp{};
    }

    auto TerminateSchedulerTask() noexcept {

        struct TerminateSchedulerOp {
            constexpr bool await_ready() const noexcept { return false; }
            KernelTaskHdl await_suspend(TaskHdl hdl) const noexcept {
                using namespace ak_internal;

                assert(gKernel.currentTaskHdl == gKernel.schedulerTaskHdl);
                assert(gKernel.currentTaskHdl == hdl);

                TaskContext& schedulerPromise = gKernel.schedulerTaskHdl.promise();
                assert(schedulerPromise.state == TaskState::RUNNING);
                assert(IsLinkDetached(&schedulerPromise.waitLink));

                schedulerPromise.state = TaskState::ZOMBIE;
                ClearTaskHdl(&gKernel.currentTaskHdl);
                EnqueueLink(&gKernel.zombieList, &schedulerPromise.waitLink);
                ++gKernel.zombieCount;

                return gKernel.kernelTask;
            }
            constexpr void await_resume() const noexcept {}
        };
        
        return TerminateSchedulerOp{};
    }

    void DestroySchedulerTask(TaskHdl hdl) noexcept {
        using namespace ak_internal;
        TaskContext* promise = &hdl.promise();

        // Remove from Task list
        DetachLink(&promise->taskListLink);
        --gKernel.taskCount;

        // Remove from Zombie List
        DetachLink(&promise->waitLink);
        --gKernel.zombieCount;

        promise->state = TaskState::DELETING; //TODO: double check
        hdl.destroy();
    }

    struct KernelTaskPromise {
        KernelTaskPromise(std::function<DefineTask()> mainProc) : mainProc(mainProc) {}
        
        void* operator new(std::size_t n) noexcept {
            void* mem = std::malloc(n);
            if (!mem) return nullptr;
            return mem;
        }

        void  operator delete(void* ptr, std::size_t sz) {
            (void)sz;
            std::free(ptr);
        }

        KernelTaskHdl  get_return_object() noexcept   { return KernelTaskHdl::from_promise(*this); }

        constexpr auto initial_suspend() noexcept     { return std::suspend_always {}; }
        constexpr auto final_suspend() noexcept       { return std::suspend_never  {}; }
        constexpr void return_void() noexcept         {}
        constexpr void unhandled_exception() noexcept { assert(false); }

        std::function<DefineTask()> mainProc;
    };

    struct DefineKernelTask {
        using promise_type = KernelTaskPromise;

        DefineKernelTask(const KernelTaskHdl& hdl) noexcept : hdl(hdl) {} 
        operator KernelTaskHdl() const noexcept { return hdl; }

        KernelTaskHdl hdl;
    };

    inline DefineTask SchedulerTaskProc(std::function<DefineTask()> mainProc) noexcept {
        using namespace ak_internal;

        TaskHdl mainTask = mainProc();
        assert(!mainTask.done());
        assert(GetTaskState(mainTask) == TaskState::READY);

        DebugTaskCount();

        while (true) {

            // If we have a ready task, resume it
            if (gKernel.readyCount > 0) {
                Link* nextNode = gKernel.readyList.prev;
                TaskContext* nextPromise = waitListNodeToTaskPromise(nextNode);
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
                TaskContext& zombiePromise = *waitListNodeToTaskPromise(zombieNode);
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
        co_await TerminateSchedulerTask();

        assert(false); // Unreachale
        co_return;
    }

    inline DefineKernelTask KernelTaskProc(std::function<DefineTask()> mainProc) noexcept {

        TaskHdl schedulerHdl = SchedulerTaskProc(mainProc);
        gKernel.schedulerTaskHdl = schedulerHdl;

        co_await RunSchedulerTask();
        DestroySchedulerTask(schedulerHdl);
        DebugTaskCount();

        co_return;
    }

    inline void InitKernel() noexcept {
        using namespace ak_internal;
        gKernel.taskCount = 0;
        gKernel.readyCount = 0;
        gKernel.waitingCount = 0;
        gKernel.ioWaitingCount = 0;
        gKernel.zombieCount = 0;
        gKernel.interrupted = 0;
        ClearTaskHdl(&gKernel.currentTaskHdl);
        ClearTaskHdl(&gKernel.schedulerTaskHdl);
        InitLink(&gKernel.zombieList);
        InitLink(&gKernel.readyList);
        InitLink(&gKernel.taskList);
    }

    inline void FiniKernel() noexcept { }

    inline void InitialSuspendTaskOp::await_suspend(TaskHdl hdl) const noexcept {
        TaskContext* promise = &hdl.promise();

        // Check initial preconditions
        assert(promise->state == TaskState::CREATED);
        assert(IsLinkDetached(&promise->waitLink));
        CheckInvariants();

        // Add task to the kernel
        ++gKernel.taskCount;
        EnqueueLink(&gKernel.taskList, &promise->taskListLink);

        ++gKernel.readyCount;
        EnqueueLink(&gKernel.readyList, &promise->waitLink);
        promise->state = TaskState::READY;

        // Check post-conditions
        assert(promise->state == TaskState::READY);
        assert(!IsLinkDetached(&promise->waitLink));
        CheckInvariants();
        ak_internal::DebugTaskCount();
    }

    inline TaskHdl FinalSuspendTaskOp::await_suspend(TaskHdl currentTaskHdl) const noexcept {
        // Check preconditions
        TaskContext* currentPromise = &currentTaskHdl.promise();
        assert(gKernel.currentTaskHdl == currentTaskHdl);
        assert(currentPromise->state == TaskState::RUNNING);
        assert(IsLinkDetached(&currentPromise->waitLink));
        CheckInvariants();

        // Move the current task from RUNNING to ZOMBIE
        currentPromise->state = TaskState::ZOMBIE;
        ++gKernel.zombieCount;
        EnqueueLink(&gKernel.zombieList, &currentPromise->waitLink);
        ClearTaskHdl(&gKernel.currentTaskHdl);
        CheckInvariants();

        // Move the SchedulerTask from READY to RUNNING
        TaskContext& schedulerPromise = gKernel.schedulerTaskHdl.promise();
        schedulerPromise.state = TaskState::RUNNING;
        DetachLink(&schedulerPromise.waitLink); // remove from ready list
        --gKernel.readyCount;
        gKernel.currentTaskHdl = gKernel.schedulerTaskHdl;
        CheckInvariants();

        return gKernel.schedulerTaskHdl;
    }

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
        if constexpr (IS_DEBUG_MODE) {
            DoCheckTaskCountInvariant();
        }
    }

    inline void CheckInvariants() noexcept {
        if constexpr (IS_DEBUG_MODE) {
            // check the Task invariants
            DoCheckTaskCountInvariant();

            // Ensure that the current Task is valid
            // if (gKernel.currentTask.isValid()) std::abort();
        }
    }

    inline constexpr TaskHdl JoinTaskOp::await_suspend(TaskHdl currentTaskHdl) const noexcept
    {
        TaskContext* currentTaskPromise = &currentTaskHdl.promise();
        TaskContext* hdlTaskPromise = &hdl.promise();

        // Check preconditions

        assert(currentTaskPromise->state == TaskState::RUNNING);
        assert(IsLinkDetached(&currentTaskPromise->waitLink));
        assert(gKernel.currentTaskHdl == currentTaskHdl);
        CheckInvariants();

        // Move the current task from RUNNINIG to WAITING

        currentTaskPromise->state = TaskState::WAITING;
        ++gKernel.waitingCount;
        EnqueueLink(&hdlTaskPromise->awaitingTerminationList, &currentTaskPromise->waitLink); 
        ClearTaskHdl(&gKernel.currentTaskHdl);
        CheckInvariants();
        DebugTaskCount();
        // if (hdlTaskPromise.state == TaskState::READY) {
        //     // Just move to READY
        //     return hdlTaskPromise;
        // }

        // Move the target task from READY to RUNNING
        TaskContext* schedulerPromise = &gKernel.schedulerTaskHdl.promise();
        assert(schedulerPromise->state == TaskState::READY);
        schedulerPromise->state = TaskState::RUNNING;
        DetachLink(&schedulerPromise->waitLink);
        --gKernel.readyCount;
        gKernel.currentTaskHdl = gKernel.schedulerTaskHdl;
        CheckInvariants();
        DebugTaskCount();

        return gKernel.schedulerTaskHdl;
    }

    inline TaskHdl SuspendOp::await_suspend(TaskHdl currentTask) const noexcept {
        assert(gKernel.currentTaskHdl);

        TaskContext* currentPromise = &currentTask.promise();

        if constexpr (IS_DEBUG_MODE) {
            assert(gKernel.currentTaskHdl == currentTask);
            assert(currentPromise->state == TaskState::RUNNING);
            assert(IsLinkDetached(&currentPromise->waitLink));
            CheckInvariants();
        }

        // Move the current task from RUNNINIG to READY
        currentPromise->state = TaskState::READY;
        ++gKernel.readyCount;
        EnqueueLink(&gKernel.readyList, &currentPromise->waitLink);
        ClearTaskHdl(&gKernel.currentTaskHdl);
        CheckInvariants();

        // Resume the SchedulerTask
        TaskContext* schedulerPromise = &gKernel.schedulerTaskHdl.promise();
        schedulerPromise->state = TaskState::RUNNING;
        DetachLink(&schedulerPromise->waitLink); // remove from ready list
        --gKernel.readyCount;
        gKernel.currentTaskHdl = gKernel.schedulerTaskHdl;
        CheckInvariants();

        assert(gKernel.currentTaskHdl);
        return TaskHdl::from_promise(*schedulerPromise);
    }

    inline TaskHdl ResumeTaskOp::await_suspend(TaskHdl currentTaskHdl) const noexcept {
        assert(gKernel.currentTaskHdl == currentTaskHdl);

        // Check the current Task
        TaskContext* currentPromise = &gKernel.currentTaskHdl.promise();
        assert(IsLinkDetached(&currentPromise->waitLink));
        assert(currentPromise->state == TaskState::RUNNING);
        CheckInvariants();

        // Suspend the current Task
        currentPromise->state = TaskState::READY;
        ++gKernel.readyCount;
        EnqueueLink(&gKernel.readyList, &currentPromise->waitLink);
        ClearTaskHdl(&gKernel.currentTaskHdl);
        CheckInvariants();

        // Move the target task from READY to RUNNING
        TaskContext* promise = &hdl.promise();
        promise->state = TaskState::RUNNING;
        DetachLink(&promise->waitLink);
        --gKernel.readyCount;
        gKernel.currentTaskHdl = hdl;
        CheckInvariants();

        assert(gKernel.currentTaskHdl);
        return hdl;
    }

}

/// \brief Runs the main task
/// \param UserProc the user's main task
/// \return 0 on success
/// \ingroup Kernel
inline int RunMain(std::function<DefineTask()> mainProc) noexcept {
    using namespace ak_internal;

    InitKernel();

    KernelTaskHdl hdl = KernelTaskProc(mainProc);
    gKernel.kernelTask = hdl;
    hdl.resume();

    FiniKernel();

    return 0;
}

inline void TaskContext::return_void() noexcept {
    using namespace ak_internal;
    CheckInvariants();

    // Wake up all tasks waiting for this task
    if (IsLinkDetached(&awaitingTerminationList)) {
        return;
    }

    do {
        Link* next = DequeueLink(&awaitingTerminationList);
        TaskContext* nextPromise = waitListNodeToTaskPromise(next);
        DebugTaskCount();
        assert(nextPromise->state == TaskState::WAITING);
        --gKernel.waitingCount;
        nextPromise->state = TaskState::READY;
        EnqueueLink(&gKernel.readyList, &nextPromise->waitLink);
        ++gKernel.readyCount;
        DebugTaskCount();

    } while (!IsLinkDetached(&awaitingTerminationList));
}

namespace ak_internal {
    struct Kernel gKernel;
}

// TODO: Add IO Ring
// TODO: Add Concurrency Tools

// struct Condition 
// {
//     struct WaitOp {
//         explicit WaitOp(Condition& condition) : condition(condition) {}
//         constexpr bool await_ready() const noexcept {
//             return IsTaskDone(condition.lockingTask);
//         }
//         constexpr TaskHdl await_suspend(TaskHdl currentTaskHdl) const noexcept {
//             using namespace ak_internal;
//             TaskPromise& currentTaskPromise = gKernel.currentTaskHdl.promise();
//             assert(gKernel.currentTaskHdl == currentTaskHdl);
//             // Check the current Task
//             assert(IsLinkDetached(&currentTaskPromise.waitLink));
//             assert(currentTaskPromise.state == TaskState::RUNNING);
//             ak_internal::CheckInvariants();
//             // Move the current task from READY to WAITING into the condition
//             currentTaskPromise.state = TaskState::WAITING;
//             ++gKernel.waitingCount;
//             EnqueueLink(&condition.waitNode, &currentTaskPromise.waitLink);
//             ClearTaskHdl(&gKernel.currentTaskHdl);
//             ak_internal::CheckInvariants();
//             // Move the target task from READY to RUNNING
//             TaskPromise& schedulerPromise = gKernel.schedulerTaskHdl.promise();
//             schedulerPromise.state = TaskState::RUNNING;
//             DetachLink(&schedulerPromise.waitLink); // remove from ready list
//             --gKernel.readyCount;
//             gKernel.currentTaskHdl = gKernel.schedulerTaskHdl;
//             ak_internal::CheckInvariants();
//             return gKernel.schedulerTaskHdl;
//         }
//         constexpr void await_resume() const noexcept {}
//         Condition& condition;
//     };
//     Condition(bool signaled = false) : signaled(signaled) {
//         using namespace ak_internal;
//         InitLink(&waitNode);
//     }
//     int signal() {
//         int signalled = 0;
//         // while (!wait_node.detached()) {
//         //     DList* next_waiting_task = wait_node.pop_front();
//         //     TaskPromise* next_waiting_task_promise = waitListNodeToTask(next_waiting_task);
//         //     assert(next_waiting_task_promise->state == TaskState::WAITING);
//         //     --g_kernel.waiting_count;
//         //     next_waiting_task_promise->state = TaskState::READY;
//         //     g_kernel.ready_list.push_back(&next_waiting_task_promise->wait_node);
//         //     --g_kernel.ready_count;
//         //     ++signalled;
//         // }
//         return signalled;
//     }
//     WaitOp wait() { return WaitOp(*this); }
//     void reset(bool signaled = false) {
//         using namespace ak_internal;
//         this->signaled = signaled;
//         ClearTaskHdl(&lockingTask);
//         InitLink(&waitNode);
//     }
//     // g_kernel.current_task_hdl = TaskHdl();
//     // g_kernel.current_task.clear();
//     bool              signaled = false;
//     TaskHdl           lockingTask;
//     ak_internal::Link waitNode;
// };

