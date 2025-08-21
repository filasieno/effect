#pragma once
#include <print>
#include "ak/tsk/tsk_api.hpp" // IWYU pragma: keep

namespace ak { namespace priv {
    struct RunSchedulerOp;
    struct TerminateSchedulerOp;
            
    // Boot routines
    template <typename... Args>
    BootCThread boot_main_proc(CThread(*main_proc)(Args ...) noexcept, Args ... args) noexcept;

    template <typename... Args>
    CThread scheduler_main_proc(CThread(*main_proc)(Args ...) noexcept, Args... args) noexcept;
    
    // Scheduler task routines
    constexpr RunSchedulerOp       run_scheduler() noexcept;
    constexpr TerminateSchedulerOp terminate_scheduler() noexcept;
    constexpr Void                 destroy_scheduler(CThread hdl) noexcept;
    
}}

// ================================================================================================================
// Ops definition details
// ================================================================================================================

namespace ak { namespace priv {

    struct RunSchedulerOp {
        constexpr Bool await_ready() const noexcept { return false; }
        constexpr Void await_resume() const noexcept { }
        CThread::Hdl   await_suspend(BootCThread::Hdl current_task_hdl) const noexcept;
    };

    struct TerminateSchedulerOp {
        constexpr Bool   await_ready() const noexcept { return false; }
        constexpr Void   await_resume() const noexcept { }
        BootCThread::Hdl await_suspend(CThread::Hdl hdl) const noexcept;
    };

} }


// ================================================================================================================
// Implementation
// ================================================================================================================

namespace ak { 

    inline int init_kernel(KernelConfig* config) noexcept {
        using namespace priv;
        
        if (init_alloc_table(config->mem, config->memSize) != 0) {
            return -1;
        }

        int res = io_uring_queue_init(config->ioEntryCount, &global_kernel_state.io_uring_state, 0);
        if (res < 0) {
            std::print("io_uring_queue_init failed\n");
            return -1;
        }

        global_kernel_state.mem = config->mem;
        global_kernel_state.mem_size = config->memSize;
        global_kernel_state.cthread_count = 0;
        global_kernel_state.ready_cthread_count = 0;
        global_kernel_state.waiting_cthread_count = 0;
        global_kernel_state.iowaiting_cthread_count = 0;
        global_kernel_state.zombie_cthread_count = 0;
        global_kernel_state.interrupted = 0;

        global_kernel_state.current_cthread.reset();
        global_kernel_state.scheduler_cthread.reset();

        init_dlink(&global_kernel_state.zombie_list);
        init_dlink(&global_kernel_state.ready_list);
        init_dlink(&global_kernel_state.cthread_list);
        
        return 0;
    }

    inline Void fini_kernel() noexcept {
        io_uring_queue_exit(&global_kernel_state.io_uring_state);
    }
    
    // Boot routines:
    // ----------------------------------------------------------------------------------------------------------------
    // The entry point creates the boot KernelTask
    // The KernelTask creates the Scheduler Task
    // The SChedulerTask executues the users mainProc

    template <typename... Args>
    inline int run_main(CThread(*main_proc)(Args ...) noexcept , Args... args) noexcept {
        using namespace priv;

        auto boot_cthread = boot_main_proc(main_proc, std::forward<Args>(args) ...);
        global_kernel_state.boot_cthread = boot_cthread;
        boot_cthread.hdl.resume();

        return global_kernel_state.main_cthread_exit_code;
    }

    namespace priv {

        template <typename... Args>
        inline BootCThread boot_main_proc(CThread(*main_proc)(Args ...) noexcept, Args ... args) noexcept {

            CThread::Hdl scheduler_hdl = scheduler_main_proc(main_proc, std::forward<Args>(args) ... );
            global_kernel_state.scheduler_cthread = scheduler_hdl;

            co_await run_scheduler();
            destroy_scheduler(scheduler_hdl);
            dump_task_count();

            co_return;
        }

        template <typename... Args>
        inline CThread scheduler_main_proc(CThread(*main_proc)(Args ...) noexcept, Args... args) noexcept {
            using namespace priv;

            CThread::Hdl main_task = main_proc(args...);
            global_kernel_state.main_cthread = main_task;
            AK_ASSERT(!main_task.done());
            AK_ASSERT(get_state(main_task) == CThread::State::READY);

            while (true) {
                // Sumbit IO operations
                unsigned ready = io_uring_sq_ready(&global_kernel_state.io_uring_state);
                if (ready > 0) {
                    int ret = io_uring_submit(&global_kernel_state.io_uring_state);
                    if (ret < 0) {
                        std::print("io_uring_submit failed\n");
                        fflush(stdout);
                        abort();
                    }
                }

                // If we have a ready task, resume it
                if (global_kernel_state.ready_cthread_count > 0) {
                    priv::DLink* next_node = global_kernel_state.ready_list.prev;
                    CThread::Context* next_promise = get_linked_cthread_context(next_node);
                    CThread::Hdl next_task = CThread::Hdl::from_promise(*next_promise);
                    AK_ASSERT(next_task != global_kernel_state.scheduler_cthread);
                    co_await op::ResumeCThread(next_task);
                    AK_ASSERT(global_kernel_state.current_cthread);
                    continue;
                }

                // Zombie bashing
                while (global_kernel_state.zombie_cthread_count > 0) {
                    dump_task_count();

                    priv::DLink* zombie_link = dequeue_dlink(&global_kernel_state.zombie_list);
                    CThread::Context* ctx = get_linked_cthread_context(zombie_link);
                    AK_ASSERT(ctx->state == CThread::State::ZOMBIE);

                    // Remove from zombie list
                    --global_kernel_state.zombie_cthread_count;
                    detach_dlink(&ctx->wait_link);

                    // Remove from task list
                    detach_dlink(&ctx->tasklist_link);
                    --global_kernel_state.cthread_count;

                    // Delete
                    ctx->state = CThread::State::DELETING;
                    CThread::Hdl zombieTaskHdl = CThread::Hdl::from_promise(*ctx);
                    zombieTaskHdl.destroy();

                    dump_task_count();
                }

                Bool waiting_cc = global_kernel_state.iowaiting_cthread_count;
                if (waiting_cc) {
                    // Process all available completions
                    struct io_uring_cqe *cqe;
                    unsigned head;
                    unsigned completed = 0;
                    io_uring_for_each_cqe(&global_kernel_state.io_uring_state, head, cqe) {
                        // Return Result to the target Awaitable 
                        CThread::Context* ctx = (CThread::Context*) io_uring_cqe_get_data(cqe);
                        AK_ASSERT(ctx->state == CThread::State::IO_WAITING);

                        // Move the target task from IO_WAITING to READY
                        --global_kernel_state.iowaiting_cthread_count;
                        ctx->state = CThread::State::READY;
                        ++global_kernel_state.ready_cthread_count;
                        enqueue_dlink(&global_kernel_state.ready_list, &ctx->wait_link);
                        
                        // Complete operation
                        ctx->res = cqe->res;
                        --ctx->prepared_io;
                        ++completed;
                    }
                    // Mark all as seen
                    io_uring_cq_advance(&global_kernel_state.io_uring_state, completed);
                }

                if (global_kernel_state.ready_cthread_count == 0 && global_kernel_state.iowaiting_cthread_count == 0) {
                    break;
                }
            }
            co_await terminate_scheduler();
   
            std::abort(); // Unreachable
        }


    }
}



// Kernel Task implementation
// ================================================================================================================

namespace ak { 

    inline Void* BootCThread::Context::operator new(std::size_t n) noexcept {
        std::print("KernelTaskPromise::operator new with size: {}\n", n);
        AK_ASSERT(n <= sizeof(global_kernel_state.boot_cthread_frame_buffer));
        return (Void*)global_kernel_state.boot_cthread_frame_buffer;
    }
    
}




// Kernel boot implementation 
// ================================================================================================================

namespace ak { namespace priv {

// RunSchedulerTaskOp
// ----------------------------------------------------------------------------------------------------------------

inline CThread::Hdl RunSchedulerOp::await_suspend(BootCThread::Hdl current_task_hdl) const noexcept {
    using namespace priv;

    (Void)current_task_hdl;
    CThread::Context* scheduler_ctx = get_context(global_kernel_state.scheduler_cthread);

    // Check expected state post scheduler construction

    AK_ASSERT(global_kernel_state.cthread_count == 1);
    AK_ASSERT(global_kernel_state.ready_cthread_count == 1);
    AK_ASSERT(scheduler_ctx->state == CThread::State::READY);
    AK_ASSERT(!is_dlink_detached(&scheduler_ctx->wait_link));
    AK_ASSERT(global_kernel_state.current_cthread == CThread::Hdl());

    // Setup SchedulerTask for execution (from READY -> RUNNING)
    global_kernel_state.current_cthread = global_kernel_state.scheduler_cthread;
    scheduler_ctx->state = CThread::State::RUNNING;
    detach_dlink(&scheduler_ctx->wait_link);
    --global_kernel_state.ready_cthread_count;

    // Check expected state post task system bootstrap
    check_invariants();
    return global_kernel_state.scheduler_cthread;
}

// TerminateSchedulerOp
// ----------------------------------------------------------------------------------------------------------------

inline BootCThread::Hdl TerminateSchedulerOp::await_suspend(CThread::Hdl hdl) const noexcept {
    using namespace priv;

    AK_ASSERT(global_kernel_state.current_cthread == global_kernel_state.scheduler_cthread);
    AK_ASSERT(global_kernel_state.current_cthread == hdl);

    auto* scheduler_context = get_context(global_kernel_state.scheduler_cthread);
    AK_ASSERT(scheduler_context->state == CThread::State::RUNNING);
    AK_ASSERT(is_dlink_detached(&scheduler_context->wait_link));

    scheduler_context->state = CThread::State::ZOMBIE;
    global_kernel_state.current_cthread.reset();
    enqueue_dlink(&global_kernel_state.zombie_list, &scheduler_context->wait_link);
    ++global_kernel_state.zombie_cthread_count;

    return global_kernel_state.boot_cthread;
}

// Boot implementation
// ----------------------------------------------------------------------------------------------------------------
constexpr RunSchedulerOp run_scheduler() noexcept { return {}; }

constexpr TerminateSchedulerOp terminate_scheduler() noexcept { return {}; }

inline constexpr Void destroy_scheduler(CThread ct) noexcept {
    using namespace priv;
    auto* context = get_context(ct);

    // Remove from Task list
    detach_dlink(&context->tasklist_link);
    --global_kernel_state.cthread_count;

    // Remove from Zombie List
    detach_dlink(&context->wait_link);
    --global_kernel_state.zombie_cthread_count;

    context->state = CThread::State::DELETING;
    ct.hdl.destroy();
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
inline CThread::Hdl schedule_cthread() noexcept {
    using namespace priv;

    // If we have a ready task, resume it
    while (true) {
        if (global_kernel_state.ready_cthread_count > 0) {
            priv::DLink* link = dequeue_dlink(&global_kernel_state.ready_list);
            CThread::Context* ctx = get_linked_cthread_context(link);
            CThread::Hdl task = CThread::Hdl::from_promise(*ctx);
            AK_ASSERT(ctx->state == CThread::State::READY);
            ctx->state = CThread::State::RUNNING;
            --global_kernel_state.ready_cthread_count;
            global_kernel_state.current_cthread = task;
            check_invariants();
            return task;
        }

        if (global_kernel_state.iowaiting_cthread_count > 0) {
            unsigned ready = io_uring_sq_ready(&global_kernel_state.io_uring_state);
            // Submit Ready IO Operations
            if (ready > 0) {
                int ret = io_uring_submit(&global_kernel_state.io_uring_state);
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
            io_uring_for_each_cqe(&global_kernel_state.io_uring_state, head, cqe) {
                // Return Result to the target Awaitable 
                CThread::Context* ctx = (CThread::Context*) io_uring_cqe_get_data(cqe);
                AK_ASSERT(ctx->state == CThread::State::IO_WAITING);

                // Move the target task from IO_WAITING to READY
                --global_kernel_state.iowaiting_cthread_count;
                ctx->state = CThread::State::READY;
                ++global_kernel_state.ready_cthread_count;
                enqueue_dlink(&global_kernel_state.ready_list, &ctx->wait_link);
                
                // Complete operation
                ctx->res = cqe->res;
                --ctx->prepared_io;
                ++completed;
            }
            // Mark all as seen
            io_uring_cq_advance(&global_kernel_state.io_uring_state, completed);
            
            continue;
        }

        // Zombie bashing
        while (global_kernel_state.zombie_cthread_count > 0) {
            dump_task_count();

            priv::DLink* zombie_node = dequeue_dlink(&global_kernel_state.zombie_list);
            CThread::Context& zombie_promise = *get_linked_cthread_context(zombie_node);
            AK_ASSERT(zombie_promise.state == CThread::State::ZOMBIE);

            // Remove from zombie list
            --global_kernel_state.zombie_cthread_count;
            detach_dlink(&zombie_promise.wait_link);

            // Remove from task list
            detach_dlink(&zombie_promise.tasklist_link);
            --global_kernel_state.cthread_count;

            // Delete
            zombie_promise.state = CThread::State::DELETING;
            CThread::Hdl zombie_task_hdl = CThread::Hdl::from_promise(zombie_promise);
            zombie_task_hdl.destroy();

            dump_task_count();
        }

        if (global_kernel_state.ready_cthread_count == 0) {
            abort();
        }
    }
    // unreachable
    abort();
}



} } // namespace ak::priv