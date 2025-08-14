#pragma once

#include "ak/api_priv.hpp"

namespace ak {
    namespace priv 
    {
        struct RunSchedulerTaskOp;
        struct TerminateSchedulerOp;

        struct DefineKernelTask {
            using promise_type = KernelTaskPromise;

            DefineKernelTask(const KernelTaskHdl& hdl) noexcept : hdl(hdl) {}
            operator KernelTaskHdl() const noexcept { return hdl; }

            KernelTaskHdl hdl;
        };

        struct KernelTaskPromise {
            using InitialSuspend = std::suspend_always;
            using FinalSuspend = std::suspend_never;

            static void* operator new(std::size_t) noexcept;
            static void  operator delete(void*, std::size_t) noexcept {};

            template <typename... Args>
            KernelTaskPromise(DefineTask(*)(Args ...) noexcept, Args... ) noexcept {}

            KernelTaskHdl            get_return_object() noexcept { return KernelTaskHdl::from_promise(*this); }
            constexpr InitialSuspend initial_suspend() noexcept { return {}; }
            constexpr FinalSuspend   final_suspend() noexcept { return {}; }
            constexpr void           return_void() noexcept {}
            constexpr void           unhandled_exception() noexcept { std::abort(); } 
        };
        
        // Boot routines
        template <typename... Args>
        DefineKernelTask KernelTaskProc(DefineTask(*mainProc)(Args ...) noexcept, Args ... args) noexcept;

        template <typename... Args>
        DefineTask SchedulerTaskProc(DefineTask(*mainProc)(Args ...) noexcept, Args... args) noexcept;
        
        int  InitKernel(KernelConfig* config) noexcept;
        void FiniKernel() noexcept;
        

        // Scheduler task routines
        constexpr RunSchedulerTaskOp   RunSchedulerTask() noexcept;
        constexpr TerminateSchedulerOp TerminateSchedulerTask() noexcept;
        constexpr void                 DestroySchedulerTask(TaskHdl hdl) noexcept;
    
    }       
}

// ================================================================================================================
// Ops definition details
// ================================================================================================================

namespace ak { namespace priv {

    struct RunSchedulerTaskOp {
        constexpr bool await_ready() const noexcept { return false; }
        constexpr void await_resume() const noexcept { }
        TaskHdl await_suspend(KernelTaskHdl currentTaskHdl) const noexcept;
    };

    struct TerminateSchedulerOp {
        constexpr bool await_ready() const noexcept { return false; }
        constexpr void await_resume() const noexcept { }
        KernelTaskHdl  await_suspend(TaskHdl hdl) const noexcept;
    };

} }


// ================================================================================================================
// Implementation
// ================================================================================================================

namespace ak { 

    // Boot routines:
    // ----------------------------------------------------------------------------------------------------------------
    // The entry point creates the boot KernelTask
    // The KernelTask creates the Scheduler Task
    // The SChedulerTask executues the users mainProc

    template <typename... Args>
    inline int RunMain(KernelConfig* config, DefineTask(*mainProc)(Args ...) noexcept , Args... args) noexcept {
        using namespace priv;

        if (InitKernel(config) < 0) {
            return -1;
        }

        KernelTaskHdl hdl = KernelTaskProc(mainProc, std::forward<Args>(args) ...);
        gKernel.kernelTask = hdl;
        hdl.resume();

        FiniKernel();

        return 0;
    }

    namespace priv {

        template <typename... Args>
        inline DefineKernelTask KernelTaskProc(DefineTask(*mainProc)(Args ...) noexcept, Args ... args) noexcept {

            TaskHdl schedulerHdl = SchedulerTaskProc(mainProc, std::forward<Args>(args) ... );
            gKernel.schedulerTaskHdl = schedulerHdl;

            co_await RunSchedulerTask();
            DestroySchedulerTask(schedulerHdl);
            DebugTaskCount();

            co_return;
        }

        template <typename... Args>
        inline DefineTask SchedulerTaskProc(DefineTask(*mainProc)(Args ...) noexcept, Args... args) noexcept {
            using namespace priv;

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
                        fflush(stdout);
                        abort();
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


        inline int InitKernel(KernelConfig* config) noexcept {
            using namespace priv;
            
            if (InitAllocTable(config->mem, config->memSize) != 0) {
                return -1;
            }

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
    }
}

namespace ak { namespace priv {

// KernelTaskPromise
// ----------------------------------------------------------------------------------------------------------------

inline void* KernelTaskPromise::operator new(std::size_t n) noexcept {
    std::print("KernelTaskPromise::operator new with size: {}\n", n);
    assert(n <= sizeof(gKernel.bootTaskFrame));
    return (void*)gKernel.bootTaskFrame;
}

// RunSchedulerTaskOp
// ----------------------------------------------------------------------------------------------------------------

inline TaskHdl RunSchedulerTaskOp::await_suspend(KernelTaskHdl currentTaskHdl) const noexcept {
    using namespace priv;

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

// TerminateSchedulerOp
// ----------------------------------------------------------------------------------------------------------------

inline KernelTaskHdl TerminateSchedulerOp::await_suspend(TaskHdl hdl) const noexcept {
    using namespace priv;

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

// Boot implementation
// ----------------------------------------------------------------------------------------------------------------
constexpr RunSchedulerTaskOp   RunSchedulerTask() noexcept       { return {}; }

constexpr TerminateSchedulerOp TerminateSchedulerTask() noexcept { return {}; }

inline constexpr void DestroySchedulerTask(TaskHdl hdl) noexcept {
    using namespace priv;
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

// Scheduler implementation routines
// ----------------------------------------------------------------------------------------------------------------

/// \brief Schedules the next task
/// 
/// Used in Operations to schedule the next task.
/// Assumes that the current task has been already suspended (moved to READY, WAITING, IO_WAITING, ...)
///
/// \return the next Task to be resumed
/// \internal
inline TaskHdl ScheduleNextTask() noexcept {
    using namespace priv;

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
                    fflush(stdout);
                    abort();
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
            abort();
        }
    }
    // unreachable
    abort();
}



} } // namespace ak::priv