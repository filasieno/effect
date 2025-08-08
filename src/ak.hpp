#pragma once

#include <cassert>
#include <coroutine>
#include <print>
#include <functional>
#include "liburing.h"

namespace ak {
namespace internal {
#ifdef NDEBUG
    constexpr bool IS_DEBUG_MODE    = false;
#else
    constexpr bool IS_DEBUG_MODE    = true;   
#endif
    constexpr bool TRACE_DEBUG_CODE = false;
}

// -----------------------------------------------------------------------------

/// \defgroup Task Task API
/// \brief Task API defines the API for creating and managing tasks.

/// \defgroup Kernel Kernel API
/// \brief Kernel API defines system level APIs.

// -----------------------------------------------------------------------------

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

const char* ToString(TaskState state) noexcept 
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

/// \brief Defines a Task function
/// \ingroup Task
template <typename... Args>
using TaskFn = std::function<DefineTask(Args...)>;

namespace internal {

    struct KernelTaskPromise;
    using KernelTaskHdl = std::coroutine_handle<KernelTaskPromise>;

    struct DLink {
        DLink* next;
        DLink* prev;
    };

    inline void InitLink(DLink* link) {
        link->next = link;
        link->prev = link;
    }

    inline bool IsLinkDetached(const DLink* link) {
        assert(link != nullptr);
        assert(link->next != nullptr);
        assert(link->prev != nullptr);
        return link->next == link && link->prev == link;
    }

    inline void DetachLink(DLink* link) {
        assert(link != nullptr);
        assert(link->next != nullptr);
        assert(link->prev != nullptr);
        if (IsLinkDetached(link)) return;
        link->next->prev = link->prev;
        link->prev->next = link->next;
        link->next = link;
        link->prev = link;
    }

    inline void EnqueueLink(DLink* queue, DLink* link) {
        assert(link != nullptr);
        assert(queue->next != nullptr);
        assert(queue->prev != nullptr);
        assert(IsLinkDetached(link));

        link->next = queue->next;
        link->prev = queue;

        link->next->prev = link;
        queue->next = link;
    }

    inline DLink* DequeueLink(DLink* queue) {
        assert(queue->next != nullptr);
        assert(queue->prev != nullptr);
        if (IsLinkDetached(queue)) return nullptr;
        DLink* target = queue->prev;
        DetachLink(target);
        return target;
    }

    struct Kernel {
        // Hot scheduling data (first cache line)
        TaskHdl currentTaskHdl;
        TaskHdl schedulerTaskHdl;
        DLink    readyList;
        DLink    taskList;
        void*   mem;
        Size    memSize;
        
        // IO
        alignas(64) 
        io_uring ioRing;
        unsigned ioEntryCount;
        
        // Cold data
        alignas(64) 
        KernelTaskHdl kernelTask;
        DLink          zombieList;

         // Counters (second cache line)  
        alignas(64) int taskCount;
        int readyCount;
        int waitingCount;
        int ioWaitingCount;
        int zombieCount;
        int interrupted;
    };
    
    extern struct Kernel gKernel;

    // void DebugTaskCount() noexcept;
    // void CheckInvariants() noexcept;

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
        explicit JoinTaskOp(TaskHdl hdl) : joinedTaskHdl(hdl) {};

        constexpr bool await_ready() const noexcept { return false; }
        constexpr TaskHdl await_suspend(TaskHdl currentTaskHdl) const noexcept;
        constexpr void await_resume() const noexcept {}

        TaskHdl joinedTaskHdl;
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
    using Link = internal::DLink;

    void* operator new(std::size_t n) noexcept {
        void* mem = std::malloc(n);
        if (!mem) return nullptr;
        return mem;
    }

    void  operator delete(void* ptr, std::size_t sz) {
        (void)sz;
        std::free(ptr);
    }

    template <typename... Args>
    TaskContext(Args&&... ) {
        using namespace internal;

        InitLink(&taskListLink);
        InitLink(&waitLink);
        InitLink(&awaitingTerminationList);
        state = TaskState::CREATED;
        enqueuedIO = 0;
        ioResult = -1;

        // Check post-conditions
        assert(IsLinkDetached(&taskListLink));
        assert(IsLinkDetached(&waitLink));
        assert(state == TaskState::CREATED);
        CheckInvariants();
    }

    ~TaskContext() {
        using namespace internal;
        assert(state == TaskState::DELETING);
        assert(IsLinkDetached(&taskListLink));
        assert(IsLinkDetached(&waitLink));
        DebugTaskCount();
        CheckInvariants();
    }

    TaskHdl        get_return_object() noexcept    { return TaskHdl::from_promise(*this);}
    constexpr auto initial_suspend() noexcept      { return internal::InitialSuspendTaskOp{}; }
    constexpr auto final_suspend() noexcept        { return internal::FinalSuspendTaskOp{}; }
    void           return_void() noexcept;
    void           unhandled_exception() noexcept  { assert(false); }

    TaskState state;
    int       ioResult;
    unsigned  enqueuedIO;
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
inline TaskContext* GetTaskContext(TaskHdl hdl) {
    return &hdl.promise();
}

/// \brief Get the current Task
/// \return [Async] TaskHdl
/// \ingroup Task
inline constexpr auto GetCurrentTask() noexcept {
    return internal::GetCurrentTaskOp{};
}

/// \brief Suspends the current Task and resumes the Scheduler.
/// \return [Async] void
/// \ingroup Task
inline constexpr auto SuspendTask() noexcept { return internal::SuspendOp{}; }

/// \brief Suspends the current Task until the target Task completes.
/// \param hdl a handle to the target Task.
/// \return [Async] void
/// \ingroup Task
inline auto JoinTask(TaskHdl hdl) noexcept {
	return internal::JoinTaskOp{hdl};
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
	return internal::ResumeTaskOp{hdl};
}

/// \brief Configuration for the Kernel
/// \ingroup Kernel
struct KernelConfig {
    void*    mem;
    Size     memSize;
    unsigned ioEntryCount;
};

// Task::AwaitTaskEffect
// ----------------------------------------------------------------------------------------------------------------

namespace internal 
{
    
    inline static TaskContext* GetLinkedTaskContext(const DLink* link) noexcept {
        unsigned long long promise_off = ((unsigned long long)link) - offsetof(TaskContext, waitLink);
        return (TaskContext*)promise_off;
    }

    auto RunSchedulerTask() noexcept {
        struct RunSchedulerTaskOp {

            constexpr bool await_ready() const noexcept  { return false; }
            constexpr void await_resume() const noexcept {}  

            TaskHdl await_suspend(KernelTaskHdl currentTaskHdl) const noexcept {
                using namespace internal;

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
                using namespace internal;

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
        using namespace internal;
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
        
        template <typename... Args>
        KernelTaskPromise(DefineTask(*)(Args ...) noexcept, Args... ) noexcept {}
        
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
        constexpr void return_void() noexcept         { }
        constexpr void unhandled_exception() noexcept { assert(false); }
    };

    struct DefineKernelTask {
        using promise_type = KernelTaskPromise;

        DefineKernelTask(const KernelTaskHdl& hdl) noexcept : hdl(hdl) {} 
        operator KernelTaskHdl() const noexcept { return hdl; }

        KernelTaskHdl hdl;
    };

    /// \brief Schedules the next task
    /// 
    /// Used in Operations to schedule the next task.
    /// Assumes that the current task has been already suspended (moved to READY, WAITING, IO_WAITING, ...)
    ///
    /// \return the next Task to be resumed
    /// \internal
    TaskHdl ScheduleNextTask() noexcept {
        using namespace internal;

        // If we have a ready task, resume it
        while (true) {
            if (gKernel.readyCount > 0) {
                DLink* link = DequeueLink(&gKernel.readyList);
                TaskContext* ctx = GetLinkedTaskContext(link);
                TaskHdl task = TaskHdl::from_promise(*ctx);
                assert(ctx->state == TaskState::READY);
                ctx->state = TaskState::RUNNING;
                --gKernel.readyCount;
                gKernel.currentTaskHdl = task;
                CheckInvariants();
                return task;
            }

            if (gKernel.ioWaitingCount > 0) {
                unsigned ready = io_uring_sq_ready(&gKernel.ioRing);
                // Submit Ready IO Operations
                if (ready > 0) {
                    int ret = io_uring_submit(&gKernel.ioRing);
                    if (ret < 0) {
                        std::print("io_uring_submit failed\n");
                        std::fflush(stdout);
                        std::abort();
                    }
                }

                // Process all available completions
                struct io_uring_cqe *cqe;
                unsigned head;
                unsigned completed = 0;
                io_uring_for_each_cqe(&gKernel.ioRing, head, cqe) {
                    // Return Result to the target Awaitable 
                    TaskContext* ctx = (TaskContext*) io_uring_cqe_get_data(cqe);
                    assert(ctx->state == TaskState::IO_WAITING);

                    // Move the target task from IO_WAITING to READY
                    --gKernel.ioWaitingCount;
                    ctx->state = TaskState::READY;
                    ++gKernel.readyCount;
                    EnqueueLink(&gKernel.readyList, &ctx->waitLink);
                    
                    // Complete operation
                    ctx->ioResult = cqe->res;
                    --ctx->enqueuedIO;
                    ++completed;
                }
                // Mark all as seen
                io_uring_cq_advance(&gKernel.ioRing, completed);
                
                continue;
            }

            // Zombie bashing
            while (gKernel.zombieCount > 0) {
                DebugTaskCount();

                DLink* zombieNode = DequeueLink(&gKernel.zombieList);
                TaskContext& zombiePromise = *GetLinkedTaskContext(zombieNode);
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

            if (gKernel.readyCount == 0) {
                std::abort();
            }
        }
        // unreachable
        std::abort();
    }

    template <typename... Args>
    inline DefineTask SchedulerTaskProc(DefineTask(*mainProc)(Args ...) noexcept, Args... args) noexcept {
        using namespace internal;

        TaskHdl mainTask = mainProc(args...);
        assert(!mainTask.done());
        assert(GetTaskState(mainTask) == TaskState::READY);

        while (true) {
            // Sumbit IO operations
            unsigned ready = io_uring_sq_ready(&gKernel.ioRing);
            if (ready > 0) {
                int ret = io_uring_submit(&gKernel.ioRing);
                if (ret < 0) {
                    std::print("io_uring_submit failed\n");
                    std::fflush(stdout);
                    std::abort();
                }
            }

            // If we have a ready task, resume it
            if (gKernel.readyCount > 0) {
                DLink* nextNode = gKernel.readyList.prev;
                TaskContext* nextPromise = GetLinkedTaskContext(nextNode);
                TaskHdl nextTask = TaskHdl::from_promise(*nextPromise);
                assert(nextTask != gKernel.schedulerTaskHdl);
                co_await ResumeTaskOp(nextTask);
                assert(gKernel.currentTaskHdl);
                continue;
            }

            // Zombie bashing
            while (gKernel.zombieCount > 0) {
                DebugTaskCount();

                DLink* zombieNode = DequeueLink(&gKernel.zombieList);
                TaskContext& zombiePromise = *GetLinkedTaskContext(zombieNode);
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

            bool waitingCC = gKernel.ioWaitingCount;
            if (waitingCC) {
                // Process all available completions
                struct io_uring_cqe *cqe;
                unsigned head;
                unsigned completed = 0;
                io_uring_for_each_cqe(&gKernel.ioRing, head, cqe) {
                    // Return Result to the target Awaitable 
                    TaskContext* ctx = (TaskContext*) io_uring_cqe_get_data(cqe);
                    assert(ctx->state == TaskState::IO_WAITING);

                    // Move the target task from IO_WAITING to READY
                    --gKernel.ioWaitingCount;
                    ctx->state = TaskState::READY;
                    ++gKernel.readyCount;
                    EnqueueLink(&gKernel.readyList, &ctx->waitLink);
                    
                    // Complete operation
                    ctx->ioResult = cqe->res;
                    --ctx->enqueuedIO;
                    ++completed;
                }
                // Mark all as seen
                io_uring_cq_advance(&gKernel.ioRing, completed);
            }

            if (gKernel.readyCount == 0 && gKernel.ioWaitingCount == 0) {
                break;
            }
        }
        co_await TerminateSchedulerTask();

        assert(false); // Unreachale
        co_return;
    }

    template <typename... Args>
    inline DefineKernelTask KernelTaskProc(DefineTask(*mainProc)(Args ...) noexcept, Args ... args) noexcept {

        TaskHdl schedulerHdl = SchedulerTaskProc(mainProc,  std::forward<Args>(args) ... );
        gKernel.schedulerTaskHdl = schedulerHdl;

        co_await RunSchedulerTask();
        DestroySchedulerTask(schedulerHdl);
        DebugTaskCount();

        co_return;
    }

    inline int InitKernel(KernelConfig* config) noexcept {
        using namespace internal;
        
        int res = io_uring_queue_init(config->ioEntryCount, &gKernel.ioRing, 0);
        if (res < 0) {
            std::print("io_uring_queue_init failed\n");
            return -1;
        }

        gKernel.mem = config->mem;
        gKernel.memSize = config->memSize;
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
        
        return 0;
    }

    inline void FiniKernel() noexcept {
        io_uring_queue_exit(&gKernel.ioRing);
    }

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
        internal::DebugTaskCount();
    }

    inline TaskHdl FinalSuspendTaskOp::await_suspend(TaskHdl hdl) const noexcept {
        // Check preconditions
        TaskContext* ctx = &hdl.promise();
        assert(gKernel.currentTaskHdl == hdl);
        assert(ctx->state == TaskState::RUNNING);
        assert(IsLinkDetached(&ctx->waitLink));
        CheckInvariants();

        // Move the current task from RUNNING to ZOMBIE
        ctx->state = TaskState::ZOMBIE;
        ++gKernel.zombieCount;
        EnqueueLink(&gKernel.zombieList, &ctx->waitLink);
        ClearTaskHdl(&gKernel.currentTaskHdl);
        CheckInvariants();

        return ScheduleNextTask();
    }

    inline void DebugTaskCount() noexcept {
        if constexpr (TRACE_DEBUG_CODE) {
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
        TaskContext* currentTaskCtx = &currentTaskHdl.promise();

        // Check CurrentTask preconditions
        assert(currentTaskCtx->state == TaskState::RUNNING);
        assert(IsLinkDetached(&currentTaskCtx->waitLink));
        assert(gKernel.currentTaskHdl == currentTaskHdl);
        CheckInvariants();

        TaskContext* joinedTaskCtx = &joinedTaskHdl.promise();                
        TaskState joinedTaskState = joinedTaskCtx->state;
        switch (joinedTaskState) {
            case TaskState::READY:
            {

                // Move current Task from READY to WAITING
                currentTaskCtx->state = TaskState::WAITING;
                ++gKernel.waitingCount;
                EnqueueLink(&joinedTaskCtx->awaitingTerminationList, &currentTaskCtx->waitLink); 
                ClearTaskHdl(&gKernel.currentTaskHdl);
                CheckInvariants();
                DebugTaskCount();

                // Move the joined TASK from READY to RUNNING
                joinedTaskCtx->state = TaskState::RUNNING;
                DetachLink(&joinedTaskCtx->waitLink);
                --gKernel.readyCount;
                gKernel.currentTaskHdl = joinedTaskHdl;
                CheckInvariants();
                DebugTaskCount();
                return joinedTaskHdl;
            }

            case TaskState::IO_WAITING:
            case TaskState::WAITING:
            {
                 // Move current Task from READY to WAITING
                currentTaskCtx->state = TaskState::WAITING;
                ++gKernel.waitingCount;
                EnqueueLink(&joinedTaskCtx->awaitingTerminationList, &currentTaskCtx->waitLink); 
                ClearTaskHdl(&gKernel.currentTaskHdl);
                CheckInvariants();
                DebugTaskCount();

                // Move the Scheduler Task from READY to RUNNING
                TaskContext* schedCtx = &gKernel.schedulerTaskHdl.promise();
                assert(schedCtx->state == TaskState::READY);
                schedCtx->state = TaskState::RUNNING;
                DetachLink(&schedCtx->waitLink);
                --gKernel.readyCount;
                gKernel.currentTaskHdl = gKernel.schedulerTaskHdl;
                CheckInvariants();
                DebugTaskCount();

                return gKernel.schedulerTaskHdl;
            }
            
            case TaskState::DELETING:
            case TaskState::ZOMBIE:
            {
                return currentTaskHdl;
            }
            
            case TaskState::INVALID:
            case TaskState::CREATED:
            case TaskState::RUNNING:
            default:
            {
                // Illegal State
                std::abort();
            }
        }
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

        return ScheduleNextTask();
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
template <typename... Args>
inline int RunMain(KernelConfig* config, DefineTask(*mainProc)(Args ...) noexcept , Args... args) noexcept {  
    using namespace internal;

    if (InitKernel(config) < 0) {
        return -1;
    }

    KernelTaskHdl hdl = KernelTaskProc(mainProc, std::forward<Args>(args) ...);
    gKernel.kernelTask = hdl;
    hdl.resume();

    FiniKernel();

    return 0;
}

inline void TaskContext::return_void() noexcept {
    using namespace internal;

    CheckInvariants();

    // Wake up all tasks waiting for this task
    if (IsLinkDetached(&awaitingTerminationList)) {
        return;
    }

    do {
        Link* next = DequeueLink(&awaitingTerminationList);
        TaskContext* ctx = GetLinkedTaskContext(next);
        DebugTaskCount();
        assert(ctx->state == TaskState::WAITING);
        --gKernel.waitingCount;
        ctx->state = TaskState::READY;
        EnqueueLink(&gKernel.readyList, &ctx->waitLink);
        ++gKernel.readyCount;
        DebugTaskCount();

    } while (!IsLinkDetached(&awaitingTerminationList));
}

namespace internal {
    struct Kernel gKernel;
}

struct Event {  
    internal::DLink waitingList;
};

void InitEvent(Event* event) {
    InitLink(&event->waitingList);
}

int SignalOne(Event* event) {
    using namespace internal;
    assert(event != nullptr);
    
    if (IsLinkDetached(&event->waitingList)) return 0;

    DLink* link = DequeueLink(&event->waitingList);
    TaskContext* ctx = GetLinkedTaskContext(link);
    assert(ctx->state == TaskState::WAITING);
    
    // Move the target task from WAITING to READY
    DetachLink(link);
    --gKernel.waitingCount;
    ctx->state = TaskState::READY;
    EnqueueLink(&gKernel.readyList, &ctx->waitLink);
    ++gKernel.readyCount;
    return 1;
}

int SignalSome(Event* event, int n) {
    using namespace internal;
    assert(event != nullptr);
    assert(n >= 0);
    int cc = 0;
    while (cc < n && !IsLinkDetached(&event->waitingList)) {
        DLink* link = DequeueLink(&event->waitingList);
        TaskContext* ctx = GetLinkedTaskContext(link);
        assert(ctx->state == TaskState::WAITING);
        
        // Move the target task from WAITING to READY
        DetachLink(link);
        --gKernel.waitingCount;
        ctx->state = TaskState::READY;
        EnqueueLink(&gKernel.readyList, &ctx->waitLink);
        ++gKernel.readyCount;    
        ++cc;
    }
    return cc;
}

int SignalAll(Event* event) {
    using namespace internal;
    assert(event != nullptr);
    int signalled = 0;
    while (!IsLinkDetached(&event->waitingList)) {
        DLink* link = DequeueLink(&event->waitingList);
        TaskContext* ctx = GetLinkedTaskContext(link);
        assert(ctx->state == TaskState::WAITING);
        
        // Move the target task from WAITING to READY
        DetachLink(link);
        --gKernel.waitingCount;
        ctx->state = TaskState::READY;
        EnqueueLink(&gKernel.readyList, &ctx->waitLink);
        ++gKernel.readyCount;
        
        ++signalled;        
    }
    return signalled;
}

auto WaitEvent(Event* event) {
    using namespace internal;
    
    assert(event != nullptr);
    
    struct WaitOp {
        
        WaitOp(Event* event) : evt(event) {}

        constexpr bool await_ready() const noexcept { 
            return false; 
        }

        constexpr TaskHdl await_suspend(TaskHdl hdl) const noexcept {
            using namespace internal;

            TaskContext* ctx = &hdl.promise();
            assert(gKernel.currentTaskHdl == hdl);
            assert(ctx->state == TaskState::RUNNING);
            
            // Move state from RUNNING to WAITING  
            ctx->state = TaskState::WAITING;
            ++gKernel.waitingCount;
            EnqueueLink(&evt->waitingList, &ctx->waitLink);
            ClearTaskHdl(&gKernel.currentTaskHdl);
            CheckInvariants();

            return ScheduleNextTask();
        }

        constexpr void await_resume() const noexcept { }

        Event* evt;
    };

    return WaitOp{event};
}

// -----------------------------------------------------------------------------
// DebugAPI 
// -----------------------------------------------------------------------------

inline void DebugIOURingFeatures(const unsigned int features) {
    std::print("IO uring features:\n");
    if (features & IORING_FEAT_SINGLE_MMAP)     std::print("  SINGLE_MMAP\n");
    if (features & IORING_FEAT_NODROP)          std::print("  NODROP\n");
    if (features & IORING_FEAT_SUBMIT_STABLE)   std::print("  SUBMIT_STABLE\n");
    if (features & IORING_FEAT_RW_CUR_POS)      std::print("  RW_CUR_POS\n");
    if (features & IORING_FEAT_CUR_PERSONALITY) std::print("  CUR_PERSONALITY\n");
    if (features & IORING_FEAT_FAST_POLL)       std::print("  FAST_POLL\n");
    if (features & IORING_FEAT_POLL_32BITS)     std::print("  POLL_32BITS\n");
    if (features & IORING_FEAT_SQPOLL_NONFIXED) std::print("  SQPOLL_NONFIXED\n");
    if (features & IORING_FEAT_EXT_ARG)         std::print("  EXT_ARG\n");
    if (features & IORING_FEAT_NATIVE_WORKERS)  std::print("  NATIVE_WORKERS\n");
}

inline void DebugIOURingSetupFlags(const unsigned int flags) {
    std::print("IO uring flags:\n");
    if (flags & IORING_SETUP_IOPOLL)    std::print("  IOPOLL\n");
    if (flags & IORING_SETUP_SQPOLL)    std::print("  SQPOLL\n");
    if (flags & IORING_SETUP_SQ_AFF)    std::print("  SQ_AFF\n");
    if (flags & IORING_SETUP_CQSIZE)    std::print("  CQSIZE\n");
    if (flags & IORING_SETUP_CLAMP)     std::print("  CLAMP\n");
    if (flags & IORING_SETUP_ATTACH_WQ) std::print("  ATTACH_WQ\n");
}

inline void DebugIOURingParams(const io_uring_params* p) {
    std::print("IO uring parameters:\n");
    
    // Main parameters
    std::print("Main Configuration:\n");
    std::print("  sq_entries: {}\n", p->sq_entries);
    std::print("  cq_entries: {}\n", p->cq_entries);
    std::print("  sq_thread_cpu: {}\n", p->sq_thread_cpu);
    std::print("  sq_thread_idle: {}\n", p->sq_thread_idle);
    std::print("  wq_fd: {}\n", p->wq_fd);

    // Print flags
    DebugIOURingSetupFlags(p->flags);

    // Print features
    DebugIOURingFeatures(p->features);

    // Submission Queue Offsets

    std::print("Submission Queue Offsets:\n");
    std::print("  head: {}\n", p->sq_off.head);
    std::print("  tail: {}\n", p->sq_off.tail);
    std::print("  ring_mask: {}\n", p->sq_off.ring_mask);
    std::print("  ring_entries: {}\n", p->sq_off.ring_entries);
    std::print("  flags: {}\n", p->sq_off.flags);
    std::print("  dropped: {}\n", p->sq_off.dropped);
    std::print("  array: {}\n", p->sq_off.array);

    // Completion Queue Offsets

    std::print("Completion Queue Offsets:\n");
    std::print("  head: {}\n", p->cq_off.head);
    std::print("  tail: {}\n", p->cq_off.tail);
    std::print("  ring_mask: {}\n", p->cq_off.ring_mask);
    std::print("  ring_entries: {}\n", p->cq_off.ring_entries);
    std::print("  overflow: {}\n", p->cq_off.overflow);
    std::print("  cqes: {}\n", p->cq_off.cqes);
    std::print("  flags: {}\n", p->cq_off.flags);
    std::print("\n");
    std::fflush(stdout);
}

// -----------------------------------------------------------------------------
// IO Operators
// -----------------------------------------------------------------------------

namespace internal {

    struct IOOp {
        constexpr bool await_ready() const noexcept { 
            TaskContext* ctx = &gKernel.currentTaskHdl.promise();
            if constexpr (IS_DEBUG_MODE) {
                assert(ctx->state == TaskState::RUNNING);
                assert(IsLinkDetached(&ctx->waitLink));
                assert(ctx->enqueuedIO == 1);
                CheckInvariants();
            }

            return gKernel.currentTaskHdl.promise().ioResult != 0; 
        }
        
        constexpr TaskHdl await_suspend(TaskHdl currentTaskHdl) noexcept {
            // if suspend is called we know that the operation has been submitted
            using namespace internal;

            // Move the current Task from RUNNING to IO_WAITING
            TaskContext* ctx = &currentTaskHdl.promise();
            assert(ctx->state == TaskState::RUNNING);
            ctx->state = TaskState::IO_WAITING;
            ++gKernel.ioWaitingCount;
            ClearTaskHdl(&gKernel.currentTaskHdl);
            CheckInvariants();
            DebugTaskCount();

            // Move the scheduler task from READY to RUNNING
            TaskContext* schedCtx = &gKernel.schedulerTaskHdl.promise();
            assert(schedCtx->state == TaskState::READY);
            schedCtx->state = TaskState::RUNNING;
            DetachLink(&schedCtx->waitLink);
            --gKernel.readyCount;
            gKernel.currentTaskHdl = gKernel.schedulerTaskHdl;
            CheckInvariants();
            DebugTaskCount();

            return gKernel.schedulerTaskHdl;
        }

        constexpr int await_resume() const noexcept { return gKernel.currentTaskHdl.promise().ioResult; }
    };

    template<typename Prep>
    inline internal::IOOp PrepareIO(Prep prep) noexcept {
        using namespace internal;
        TaskContext* ctx = &gKernel.currentTaskHdl.promise();
        
        // Ensure free submission slot 
        unsigned int free_slots = io_uring_sq_space_left(&gKernel.ioRing);
        while (free_slots < 1) {
            int ret = io_uring_submit(&gKernel.ioRing);
            if (ret < 0) {
                ctx->ioResult = -1;
                return {};
            }
            free_slots = io_uring_sq_space_left(&gKernel.ioRing);
        }

        // Enqueue operation
        io_uring_sqe* sqe = io_uring_get_sqe(&gKernel.ioRing);
        io_uring_sqe_set_data(sqe, (void*) ctx);
        prep(sqe);  // Call the preparation function
        ctx->ioResult = 0;
        ++ctx->enqueuedIO;
        return {};
    }
  
}


// File Operations
inline internal::IOOp IOOpen(const char* path, int flags, mode_t mode) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_openat(sqe, AT_FDCWD, path, flags, mode);
    });
}

inline internal::IOOp IOOpenAt(int dfd, const char* path, int flags, mode_t mode) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_openat(sqe, dfd, path, flags, mode);
    });
}

inline internal::IOOp IOOpenAtDirect(int dfd, const char* path, int flags, mode_t mode, unsigned file_index) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_openat_direct(sqe, dfd, path, flags, mode, file_index);
    });
}

inline internal::IOOp IOClose(int fd) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_close(sqe, fd);
    });
}

inline internal::IOOp IOCloseDirect(unsigned file_index) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_close_direct(sqe, file_index);
    });
}

// Read Operations
inline internal::IOOp IORead(int fd, void* buf, unsigned nbytes, __u64 offset) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_read(sqe, fd, buf, nbytes, offset);
    });
}

inline internal::IOOp IOReadMultishot(int fd, unsigned nbytes, __u64 offset, int buf_group) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_read_multishot(sqe, fd, nbytes, offset, buf_group);
    });
}

inline internal::IOOp IOReadFixed(int fd, void* buf, unsigned nbytes, __u64 offset, int buf_index) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_read_fixed(sqe, fd, buf, nbytes, offset, buf_index);
    });
}

inline internal::IOOp IOReadV(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_readv(sqe, fd, iovecs, nr_vecs, offset);
    });
}

inline internal::IOOp IOReadV2(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset, int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_readv2(sqe, fd, iovecs, nr_vecs, offset, flags);
    });
}

inline internal::IOOp IOReadVFixed(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset, int flags, int buf_index) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_readv_fixed(sqe, fd, iovecs, nr_vecs, offset, flags, buf_index);
    });
}

// Write Operations
inline internal::IOOp IOWrite(int fd, const void* buf, unsigned nbytes, __u64 offset) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_write(sqe, fd, buf, nbytes, offset);
    });
}

inline internal::IOOp IOWriteFixed(int fd, const void* buf, unsigned nbytes, __u64 offset, int buf_index) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_write_fixed(sqe, fd, buf, nbytes, offset, buf_index);
    });
}

inline internal::IOOp IOWriteV(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_writev(sqe, fd, iovecs, nr_vecs, offset);
    });
}

inline internal::IOOp IOWriteV2(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset, int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_writev2(sqe, fd, iovecs, nr_vecs, offset, flags);
    });
}

inline internal::IOOp IOWriteVFixed(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset, int flags, int buf_index) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_writev_fixed(sqe, fd, iovecs, nr_vecs, offset, flags, buf_index);
    });
}

// Socket Operations
inline internal::IOOp IOAccept(int fd, struct sockaddr* addr, socklen_t* addrlen, int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_accept(sqe, fd, addr, addrlen, flags);
    });
}

inline internal::IOOp IOAcceptDirect(int fd, struct sockaddr* addr, socklen_t* addrlen, int flags, unsigned int file_index) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_accept_direct(sqe, fd, addr, addrlen, flags, file_index);
    });
}

inline internal::IOOp IOMultishotAccept(int fd, struct sockaddr* addr, socklen_t* addrlen, int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_multishot_accept(sqe, fd, addr, addrlen, flags);
    });
}

inline internal::IOOp IOMultishotAcceptDirect(int fd, struct sockaddr* addr, socklen_t* addrlen, int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_multishot_accept_direct(sqe, fd, addr, addrlen, flags);
    });
}

inline internal::IOOp IOConnect(int fd, const struct sockaddr* addr, socklen_t addrlen) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_connect(sqe, fd, addr, addrlen);
    });
}

inline internal::IOOp IOSend(int sockfd, const void* buf, size_t len, int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_send(sqe, sockfd, buf, len, flags);
    });
}

inline internal::IOOp IOSendZC(int sockfd, const void* buf, size_t len, int flags, unsigned zc_flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_send_zc(sqe, sockfd, buf, len, flags, zc_flags);
    });
}

inline internal::IOOp IOSendZCFixed(int sockfd, const void* buf, size_t len, int flags, unsigned zc_flags, unsigned buf_index) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_send_zc_fixed(sqe, sockfd, buf, len, flags, zc_flags, buf_index);
    });
}

inline internal::IOOp IOSendMsg(int fd, const struct msghdr* msg, unsigned flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_sendmsg(sqe, fd, msg, flags);
    });
}

inline internal::IOOp IOSendMsgZC(int fd, const struct msghdr* msg, unsigned flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_sendmsg_zc(sqe, fd, msg, flags);
    });
}

inline internal::IOOp IOSendMsgZCFixed(int fd, const struct msghdr* msg, unsigned flags, unsigned buf_index) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_sendmsg_zc_fixed(sqe, fd, msg, flags, buf_index);
    });
}

inline internal::IOOp IORecv(int sockfd, void* buf, size_t len, int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_recv(sqe, sockfd, buf, len, flags);
    });
}

inline internal::IOOp IORecvMultishot(int sockfd, void* buf, size_t len, int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_recv_multishot(sqe, sockfd, buf, len, flags);
    });
}

inline internal::IOOp IORecvMsg(int fd, struct msghdr* msg, unsigned flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_recvmsg(sqe, fd, msg, flags);
    });
}

inline internal::IOOp IORecvMsgMultishot(int fd, struct msghdr* msg, unsigned flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_recvmsg_multishot(sqe, fd, msg, flags);
    });
}

inline internal::IOOp IOSocket(int domain, int type, int protocol, unsigned int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_socket(sqe, domain, type, protocol, flags);
    });
}

inline internal::IOOp IOSocketDirect(int domain, int type, int protocol, unsigned file_index, unsigned int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_socket_direct(sqe, domain, type, protocol, file_index, flags);
    });
}

// Directory and Link Operations
inline internal::IOOp IOMkdir(const char* path, mode_t mode) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_mkdir(sqe, path, mode);
    });
}

inline internal::IOOp IOMkdirAt(int dfd, const char* path, mode_t mode) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_mkdirat(sqe, dfd, path, mode);
    });
}

inline internal::IOOp IOSymlink(const char* target, const char* linkpath) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_symlink(sqe, target, linkpath);
    });
}

inline internal::IOOp IOSymlinkAt(const char* target, int newdirfd, const char* linkpath) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_symlinkat(sqe, target, newdirfd, linkpath);
    });
}

inline internal::IOOp IOLink(const char* oldpath, const char* newpath, int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_link(sqe, oldpath, newpath, flags);
    });
}

inline internal::IOOp IOLinkAt(int olddfd, const char* oldpath, int newdfd, const char* newpath, int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_linkat(sqe, olddfd, oldpath, newdfd, newpath, flags);
    });
}

// File Management Operations
inline internal::IOOp IOUnlink(const char* path, int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_unlink(sqe, path, flags);
    });
}

inline internal::IOOp IOUnlinkAt(int dfd, const char* path, int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_unlinkat(sqe, dfd, path, flags);
    });
}

inline internal::IOOp IORename(const char* oldpath, const char* newpath) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_rename(sqe, oldpath, newpath);
    });
}

inline internal::IOOp IORenameAt(int olddfd, const char* oldpath, int newdfd, const char* newpath, unsigned int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_renameat(sqe, olddfd, oldpath, newdfd, newpath, flags);
    });
}

inline internal::IOOp IOSync(int fd, unsigned fsync_flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_fsync(sqe, fd, fsync_flags);
    });
}

inline internal::IOOp IOSyncFileRange(int fd, unsigned len, __u64 offset, int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_sync_file_range(sqe, fd, len, offset, flags);
    });
}

inline internal::IOOp IOFAllocate(int fd, int mode, __u64 offset, __u64 len) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_fallocate(sqe, fd, mode, offset, len);
    });
}

inline internal::IOOp IOOpenAt2(int dfd, const char* path, struct open_how* how) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_openat2(sqe, dfd, path, how);
    });
}

inline internal::IOOp IOOpenAt2Direct(int dfd, const char* path, struct open_how* how, unsigned file_index) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_openat2_direct(sqe, dfd, path, how, file_index);
    });
}

inline internal::IOOp IOStatx(int dfd, const char* path, int flags, unsigned mask, struct statx* statxbuf) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_statx(sqe, dfd, path, flags, mask, statxbuf);
    });
}

inline internal::IOOp IOFAdvise(int fd, __u64 offset, __u32 len, int advice) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_fadvise(sqe, fd, offset, len, advice);
    });
}

inline internal::IOOp IOFAdvise64(int fd, __u64 offset, off_t len, int advice) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_fadvise64(sqe, fd, offset, len, advice);
    });
}

inline internal::IOOp IOMAdvise(void* addr, __u32 length, int advice) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_madvise(sqe, addr, length, advice);
    });
}

inline internal::IOOp IOMAdvise64(void* addr, off_t length, int advice) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_madvise64(sqe, addr, length, advice);
    });
}

// Extended Attributes Operations
inline internal::IOOp IOGetXAttr(const char* name, char* value, const char* path, unsigned int len) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_getxattr(sqe, name, value, path, len);
    });
}

inline internal::IOOp IOSetXAttr(const char* name, const char* value, const char* path, int flags, unsigned int len) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_setxattr(sqe, name, value, path, flags, len);
    });
}

inline internal::IOOp IOFGetXAttr(int fd, const char* name, char* value, unsigned int len) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_fgetxattr(sqe, fd, name, value, len);
    });
}

inline internal::IOOp IOFSetXAttr(int fd, const char* name, const char* value, int flags, unsigned int len) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_fsetxattr(sqe, fd, name, value, flags, len);
    });
}

// Buffer Operations
inline internal::IOOp IOProvideBuffers(void* addr, int len, int nr, int bgid, int bid) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_provide_buffers(sqe, addr, len, nr, bgid, bid);
    });
}

inline internal::IOOp IORemoveBuffers(int nr, int bgid) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_remove_buffers(sqe, nr, bgid);
    });
}

// Polling Operations
inline internal::IOOp IOPollAdd(int fd, unsigned poll_mask) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_poll_add(sqe, fd, poll_mask);
    });
}

inline internal::IOOp IOPollMultishot(int fd, unsigned poll_mask) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_poll_multishot(sqe, fd, poll_mask);
    });
}

inline internal::IOOp IOPollRemove(__u64 user_data) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_poll_remove(sqe, user_data);
    });
}

inline internal::IOOp IOPollUpdate(__u64 old_user_data, __u64 new_user_data, unsigned poll_mask, unsigned flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_poll_update(sqe, old_user_data, new_user_data, poll_mask, flags);
    });
}

inline internal::IOOp IOEpollCtl(int epfd, int fd, int op, struct epoll_event* ev) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_epoll_ctl(sqe, epfd, fd, op, ev);
    });
}

inline internal::IOOp IOEpollWait(int fd, struct epoll_event* events, int maxevents, unsigned flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_epoll_wait(sqe, fd, events, maxevents, flags);
    });
}

// Timeout Operations
inline internal::IOOp IOTimeout(struct __kernel_timespec* ts, unsigned count, unsigned flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_timeout(sqe, ts, count, flags);
    });
}

inline internal::IOOp IOTimeoutRemove(__u64 user_data, unsigned flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_timeout_remove(sqe, user_data, flags);
    });
}

inline internal::IOOp IOTimeoutUpdate(struct __kernel_timespec* ts, __u64 user_data, unsigned flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_timeout_update(sqe, ts, user_data, flags);
    });
}

inline internal::IOOp IOLinkTimeout(struct __kernel_timespec* ts, unsigned flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_link_timeout(sqe, ts, flags);
    });
}

// Message Ring Operations
inline internal::IOOp IOMsgRing(int fd, unsigned int len, __u64 data, unsigned int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_msg_ring(sqe, fd, len, data, flags);
    });
}

inline internal::IOOp IOMsgRingCqeFlags(int fd, unsigned int len, __u64 data, unsigned int flags, unsigned int cqe_flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_msg_ring_cqe_flags(sqe, fd, len, data, flags, cqe_flags);
    });
}

inline internal::IOOp IOMsgRingFd(int fd, int source_fd, int target_fd, __u64 data, unsigned int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_msg_ring_fd(sqe, fd, source_fd, target_fd, data, flags);
    });
}

inline internal::IOOp IOMsgRingFdAlloc(int fd, int source_fd, __u64 data, unsigned int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_msg_ring_fd_alloc(sqe, fd, source_fd, data, flags);
    });
}

// Process Operations
inline internal::IOOp IOWaitId(idtype_t idtype, id_t id, siginfo_t* infop, int options, unsigned int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_waitid(sqe, idtype, id, infop, options, flags);
    });
}

// Futex Operations
inline internal::IOOp IOFutexWake(uint32_t* futex, uint64_t val, uint64_t mask, uint32_t futex_flags, unsigned int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_futex_wake(sqe, futex, val, mask, futex_flags, flags);
    });
}

inline internal::IOOp IOFutexWait(uint32_t* futex, uint64_t val, uint64_t mask, uint32_t futex_flags, unsigned int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_futex_wait(sqe, futex, val, mask, futex_flags, flags);
    });
}

inline internal::IOOp IOFutexWaitV(struct futex_waitv* futex, uint32_t nr_futex, unsigned int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_futex_waitv(sqe, futex, nr_futex, flags);
    });
}

// File Descriptor Management
inline internal::IOOp IOFixedFdInstall(int fd, unsigned int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_fixed_fd_install(sqe, fd, flags);
    });
}

inline internal::IOOp IOFilesUpdate(int* fds, unsigned nr_fds, int offset) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_files_update(sqe, fds, nr_fds, offset);
    });
}

// Shutdown Operation
inline internal::IOOp IOShutdown(int fd, int how) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_shutdown(sqe, fd, how);
    });
}

// File Truncation
inline internal::IOOp IOFTruncate(int fd, loff_t len) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_ftruncate(sqe, fd, len);
    });
}

// Command Operations
inline internal::IOOp IOCmdSock(int cmd_op, int fd, int level, int optname, void* optval, int optlen) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_cmd_sock(sqe, cmd_op, fd, level, optname, optval, optlen);
    });
}

inline internal::IOOp IOCmdDiscard(int fd, uint64_t offset, uint64_t nbytes) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_cmd_discard(sqe, fd, offset, nbytes);
    });
}

// Special Operations
inline internal::IOOp IONop() noexcept {
    return internal::PrepareIO([](io_uring_sqe* sqe) {
        io_uring_prep_nop(sqe);
    });
}

// Splice Operations
inline internal::IOOp IOSplice(int fd_in, int64_t off_in, int fd_out, int64_t off_out, unsigned int nbytes, unsigned int splice_flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_splice(sqe, fd_in, off_in, fd_out, off_out, nbytes, splice_flags);
    });
}

inline internal::IOOp IOTee(int fd_in, int fd_out, unsigned int nbytes, unsigned int splice_flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_tee(sqe, fd_in, fd_out, nbytes, splice_flags);
    });
}

// Cancel Operations
inline internal::IOOp IOCancel64(__u64 user_data, int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_cancel64(sqe, user_data, flags);
    });
}

inline internal::IOOp IOCancel(void* user_data, int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_cancel(sqe, user_data, flags);
    });
}

inline internal::IOOp IOCancelFd(int fd, unsigned int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_cancel_fd(sqe, fd, flags);
    });
}

} // namespace ak