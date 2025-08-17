#pragma once

#include "ak/api_priv.hpp"

namespace ak {
    namespace priv 
    {
        struct RunSchedulerTaskOp;
        struct TerminateSchedulerOp;
        
        struct DefineKernelTask {
            using promise_type = BootContext;

            DefineKernelTask(const BootCtxHdl& hdl) noexcept : hdl(hdl) {}
            operator BootCtxHdl() const noexcept { return hdl; }

            BootCtxHdl hdl;
        };


        
        // Boot routines
        template <typename... Args>
        DefineKernelTask KernelTaskProc(CThread(*mainProc)(Args ...) noexcept, Args ... args) noexcept;

        template <typename... Args>
        CThread SchedulerTaskProc(CThread(*mainProc)(Args ...) noexcept, Args... args) noexcept;
        
        int  InitKernel(KernelConfig* config) noexcept;
        Void FiniKernel() noexcept;
        

        // Scheduler task routines
        constexpr RunSchedulerTaskOp   RunSchedulerTask() noexcept;
        constexpr TerminateSchedulerOp TerminateSchedulerTask() noexcept;
        constexpr Void                 DestroySchedulerTask(CThreadCtxHdl hdl) noexcept;
    
    }       
}

// ================================================================================================================
// Ops definition details
// ================================================================================================================

namespace ak { namespace priv {

    struct RunSchedulerTaskOp {
        constexpr Bool await_ready() const noexcept { return false; }
        constexpr Void await_resume() const noexcept { }
        CThreadCtxHdl await_suspend(BootCtxHdl currentTaskHdl) const noexcept;
    };

    struct TerminateSchedulerOp {
        constexpr Bool await_ready() const noexcept { return false; }
        constexpr Void await_resume() const noexcept { }
        BootCtxHdl  await_suspend(CThreadCtxHdl hdl) const noexcept;
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
    inline int run_main(KernelConfig* config, CThread(*mainProc)(Args ...) noexcept , Args... args) noexcept {
        using namespace priv;

        std::memset((Void*)&gKernel, 0, sizeof(gKernel));

        if (InitKernel(config) < 0) {
            return -1;
        }

        BootCtxHdl hdl = KernelTaskProc(mainProc, std::forward<Args>(args) ...);
        gKernel.kernel_ctx_hdl = hdl;
        hdl.resume();
        FiniKernel();

        return gKernel.mainTaskReturnValue;
    }

    namespace priv {

        template <typename... Args>
        inline DefineKernelTask KernelTaskProc(CThread(*mainProc)(Args ...) noexcept, Args ... args) noexcept {

            CThreadCtxHdl schedulerHdl = SchedulerTaskProc(mainProc, std::forward<Args>(args) ... );
            gKernel.scheduler_ctx_hdl = schedulerHdl;

            co_await RunSchedulerTask();
            DestroySchedulerTask(schedulerHdl);
            DebugTaskCount();

            co_return;
        }

        template <typename... Args>
        inline CThread SchedulerTaskProc(CThread(*mainProc)(Args ...) noexcept, Args... args) noexcept {
            using namespace priv;

            CThreadCtxHdl mainTask = mainProc(args...);
            gKernel.co_main_ctx_hdl = mainTask;
            assert(!mainTask.done());
            assert(get_task_state(mainTask) == TaskState::READY);

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
                if (gKernel.ready_count > 0) {
                    utl::DLink* nextNode = gKernel.ready_list.prev;
                    CThreadContext* nextPromise = get_linked_context(nextNode);
                    CThreadCtxHdl nextTask = CThreadCtxHdl::from_promise(*nextPromise);
                    assert(nextTask != gKernel.scheduler_ctx_hdl);
                    co_await ResumeTaskOp(nextTask);
                    assert(gKernel.current_ctx_hdl);
                    continue;
                }

                // Zombie bashing
                while (gKernel.zombie_count > 0) {
                    DebugTaskCount();

                    utl::DLink* zombieNode = dequeue_link(&gKernel.zombie_list);
                    CThreadContext& zombiePromise = *get_linked_context(zombieNode);
                    assert(zombiePromise.state == TaskState::ZOMBIE);

                    // Remove from zombie list
                    --gKernel.zombie_count;
                    detach_link(&zombiePromise.wait_link);

                    // Remove from task list
                    detach_link(&zombiePromise.tasklist_link);
                    --gKernel.task_count;

                    // Delete
                    zombiePromise.state = TaskState::DELETING;
                    CThreadCtxHdl zombieTaskHdl = CThreadCtxHdl::from_promise(zombiePromise);
                    zombieTaskHdl.destroy();

                    DebugTaskCount();
                }

                Bool waitingCC = gKernel.iowaiting_count;
                if (waitingCC) {
                    // Process all available completions
                    struct io_uring_cqe *cqe;
                    unsigned head;
                    unsigned completed = 0;
                    io_uring_for_each_cqe(&gKernel.ioRing, head, cqe) {
                        // Return Result to the target Awaitable 
                        CThreadContext* ctx = (CThreadContext*) io_uring_cqe_get_data(cqe);
                        assert(ctx->state == TaskState::IO_WAITING);

                        // Move the target task from IO_WAITING to READY
                        --gKernel.iowaiting_count;
                        ctx->state = TaskState::READY;
                        ++gKernel.ready_count;
                        enqueue_link(&gKernel.ready_list, &ctx->wait_link);
                        
                        // Complete operation
                        ctx->res = cqe->res;
                        --ctx->prepared_io;
                        ++completed;
                    }
                    // Mark all as seen
                    io_uring_cq_advance(&gKernel.ioRing, completed);
                }

                if (gKernel.ready_count == 0 && gKernel.iowaiting_count == 0) {
                    break;
                }
            }
            co_await TerminateSchedulerTask();
   
            std::abort(); // Unreachable
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
            gKernel.task_count = 0;
            gKernel.ready_count = 0;
            gKernel.waiting_count = 0;
            gKernel.iowaiting_count = 0;
            gKernel.zombie_count = 0;
            gKernel.interrupted = 0;

            clear(&gKernel.current_ctx_hdl);
            clear(&gKernel.scheduler_ctx_hdl);

            utl::init_link(&gKernel.zombie_list);
            utl::init_link(&gKernel.ready_list);
            utl::init_link(&gKernel.task_list);
            
            return 0;
        }

        inline Void FiniKernel() noexcept {
            io_uring_queue_exit(&gKernel.ioRing);
        }
    }
}



// Kernel Task implementation
// ================================================================================================================

namespace ak { 

    inline Void* BootContext::operator new(std::size_t n) noexcept {
        std::print("KernelTaskPromise::operator new with size: {}\n", n);
        assert(n <= sizeof(gKernel.bootTaskFrame));
        return (Void*)gKernel.bootTaskFrame;
    }
    
}




// Kernel boot implementation 
// ================================================================================================================

namespace ak { namespace priv {

// RunSchedulerTaskOp
// ----------------------------------------------------------------------------------------------------------------

inline CThreadCtxHdl RunSchedulerTaskOp::await_suspend(BootCtxHdl currentTaskHdl) const noexcept {
    using namespace priv;

    (Void)currentTaskHdl;
    CThreadContext& schedulerPromise = gKernel.scheduler_ctx_hdl.promise();

    // Check expected state post scheduler construction

    assert(gKernel.task_count == 1);
    assert(gKernel.ready_count == 1);
    assert(schedulerPromise.state == TaskState::READY);
    assert(!is_link_detached(&schedulerPromise.wait_link));
    assert(gKernel.current_ctx_hdl == CThreadCtxHdl());

    // Setup SchedulerTask for execution (from READY -> RUNNING)
    gKernel.current_ctx_hdl = gKernel.scheduler_ctx_hdl;
    schedulerPromise.state = TaskState::RUNNING;
    detach_link(&schedulerPromise.wait_link);
    --gKernel.ready_count;

    // Check expected state post task system bootstrap
    CheckInvariants();
    return gKernel.scheduler_ctx_hdl;
}

// TerminateSchedulerOp
// ----------------------------------------------------------------------------------------------------------------

inline BootCtxHdl TerminateSchedulerOp::await_suspend(CThreadCtxHdl hdl) const noexcept {
    using namespace priv;

    assert(gKernel.current_ctx_hdl == gKernel.scheduler_ctx_hdl);
    assert(gKernel.current_ctx_hdl == hdl);

    CThreadContext& schedulerPromise = gKernel.scheduler_ctx_hdl.promise();
    assert(schedulerPromise.state == TaskState::RUNNING);
    assert(utl::is_link_detached(&schedulerPromise.wait_link));

    schedulerPromise.state = TaskState::ZOMBIE;
    clear(&gKernel.current_ctx_hdl);
    enqueue_link(&gKernel.zombie_list, &schedulerPromise.wait_link);
    ++gKernel.zombie_count;

    return gKernel.kernel_ctx_hdl;
}

// Boot implementation
// ----------------------------------------------------------------------------------------------------------------
constexpr RunSchedulerTaskOp   RunSchedulerTask() noexcept       { return {}; }

constexpr TerminateSchedulerOp TerminateSchedulerTask() noexcept { return {}; }

inline constexpr Void DestroySchedulerTask(CThreadCtxHdl hdl) noexcept {
    using namespace priv;
    CThreadContext* promise = &hdl.promise();

    // Remove from Task list
    detach_link(&promise->tasklist_link);
    --gKernel.task_count;

    // Remove from Zombie List
    detach_link(&promise->wait_link);
    --gKernel.zombie_count;

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
inline CThreadCtxHdl ScheduleNextTask() noexcept {
    using namespace priv;

    // If we have a ready task, resume it
    while (true) {
        if (gKernel.ready_count > 0) {
            utl::DLink* link = dequeue_link(&gKernel.ready_list);
            CThreadContext* ctx = get_linked_context(link);
            CThreadCtxHdl task = CThreadCtxHdl::from_promise(*ctx);
            assert(ctx->state == TaskState::READY);
            ctx->state = TaskState::RUNNING;
            --gKernel.ready_count;
            gKernel.current_ctx_hdl = task;
            CheckInvariants();
            return task;
        }

        if (gKernel.iowaiting_count > 0) {
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
                CThreadContext* ctx = (CThreadContext*) io_uring_cqe_get_data(cqe);
                assert(ctx->state == TaskState::IO_WAITING);

                // Move the target task from IO_WAITING to READY
                --gKernel.iowaiting_count;
                ctx->state = TaskState::READY;
                ++gKernel.ready_count;
                enqueue_link(&gKernel.ready_list, &ctx->wait_link);
                
                // Complete operation
                ctx->res = cqe->res;
                --ctx->prepared_io;
                ++completed;
            }
            // Mark all as seen
            io_uring_cq_advance(&gKernel.ioRing, completed);
            
            continue;
        }

        // Zombie bashing
        while (gKernel.zombie_count > 0) {
            DebugTaskCount();

            utl::DLink* zombieNode = dequeue_link(&gKernel.zombie_list);
            CThreadContext& zombiePromise = *get_linked_context(zombieNode);
            assert(zombiePromise.state == TaskState::ZOMBIE);

            // Remove from zombie list
            --gKernel.zombie_count;
            detach_link(&zombiePromise.wait_link);

            // Remove from task list
            detach_link(&zombiePromise.tasklist_link);
            --gKernel.task_count;

            // Delete
            zombiePromise.state = TaskState::DELETING;
            CThreadCtxHdl zombieTaskHdl = CThreadCtxHdl::from_promise(zombiePromise);
            zombieTaskHdl.destroy();

            DebugTaskCount();
        }

        if (gKernel.ready_count == 0) {
            abort();
        }
    }
    // unreachable
    abort();
}



} } // namespace ak::priv