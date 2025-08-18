#pragma once

#include "ak/api_priv.hpp"

namespace ak {
    namespace priv 
    {
        struct RunSchedulerOp;
        struct TerminateSchedulerOp;
                
        // Boot routines
        template <typename... Args>
        BootCThread boot_main_proc(CThread(*main_proc)(Args ...) noexcept, Args ... args) noexcept;

        template <typename... Args>
        CThread scheduler_main_proc(CThread(*main_proc)(Args ...) noexcept, Args... args) noexcept;
        
        int  init_kernel(KernelConfig* config) noexcept;
        Void fini_kernel() noexcept;
        
        // Scheduler task routines
        constexpr RunSchedulerOp       run_scheduler() noexcept;
        constexpr TerminateSchedulerOp terminate_scheduler() noexcept;
        constexpr Void                 destroy_scheduler(CThread hdl) noexcept;
    
    }       
}

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

    // Boot routines:
    // ----------------------------------------------------------------------------------------------------------------
    // The entry point creates the boot KernelTask
    // The KernelTask creates the Scheduler Task
    // The SChedulerTask executues the users mainProc

    template <typename... Args>
    inline int run_main_cthread(KernelConfig* config, CThread(*main_proc)(Args ...) noexcept , Args... args) noexcept {
        using namespace priv;

        std::memset((Void*)&global_kernel_state, 0, sizeof(global_kernel_state));

        if (init_kernel(config) < 0) {
            return -1;
        }

        auto boot_cthread = boot_main_proc(main_proc, std::forward<Args>(args) ...);
        global_kernel_state.boot_cthread = boot_cthread;
        boot_cthread.hdl.resume();
        fini_kernel();

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
            assert(!main_task.done());
            assert(get_state(main_task) == CThread::State::READY);

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
                    utl::DLink* next_node = global_kernel_state.ready_list.prev;
                    CThread::Context* next_promise = get_linked_cthread_context(next_node);
                    CThread::Hdl next_task = CThread::Hdl::from_promise(*next_promise);
                    assert(next_task != global_kernel_state.scheduler_cthread);
                    co_await op::ResumeCThread(next_task);
                    assert(global_kernel_state.current_cthread);
                    continue;
                }

                // Zombie bashing
                while (global_kernel_state.zombie_cthread_count > 0) {
                    dump_task_count();

                    utl::DLink* zombie_link = dequeue_link(&global_kernel_state.zombie_list);
                    CThread::Context* ctx = get_linked_cthread_context(zombie_link);
                    assert(ctx->state == CThread::State::ZOMBIE);

                    // Remove from zombie list
                    --global_kernel_state.zombie_cthread_count;
                    detach_link(&ctx->wait_link);

                    // Remove from task list
                    detach_link(&ctx->tasklist_link);
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
                        assert(ctx->state == CThread::State::IO_WAITING);

                        // Move the target task from IO_WAITING to READY
                        --global_kernel_state.iowaiting_cthread_count;
                        ctx->state = CThread::State::READY;
                        ++global_kernel_state.ready_cthread_count;
                        enqueue_link(&global_kernel_state.ready_list, &ctx->wait_link);
                        
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

            utl::init_link(&global_kernel_state.zombie_list);
            utl::init_link(&global_kernel_state.ready_list);
            utl::init_link(&global_kernel_state.cthread_list);
            
            return 0;
        }

        inline Void fini_kernel() noexcept {
            io_uring_queue_exit(&global_kernel_state.io_uring_state);
        }
    }
}



// Kernel Task implementation
// ================================================================================================================

namespace ak { 

    inline Void* BootCThread::Context::operator new(std::size_t n) noexcept {
        std::print("KernelTaskPromise::operator new with size: {}\n", n);
        assert(n <= sizeof(global_kernel_state.boot_cthread_frame_buffer));
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

    assert(global_kernel_state.cthread_count == 1);
    assert(global_kernel_state.ready_cthread_count == 1);
    assert(scheduler_ctx->state == CThread::State::READY);
    assert(!is_link_detached(&scheduler_ctx->wait_link));
    assert(global_kernel_state.current_cthread == CThread::Hdl());

    // Setup SchedulerTask for execution (from READY -> RUNNING)
    global_kernel_state.current_cthread = global_kernel_state.scheduler_cthread;
    scheduler_ctx->state = CThread::State::RUNNING;
    detach_link(&scheduler_ctx->wait_link);
    --global_kernel_state.ready_cthread_count;

    // Check expected state post task system bootstrap
    check_invariants();
    return global_kernel_state.scheduler_cthread;
}

// TerminateSchedulerOp
// ----------------------------------------------------------------------------------------------------------------

inline BootCThread::Hdl TerminateSchedulerOp::await_suspend(CThread::Hdl hdl) const noexcept {
    using namespace priv;

    assert(global_kernel_state.current_cthread == global_kernel_state.scheduler_cthread);
    assert(global_kernel_state.current_cthread == hdl);

    auto* scheduler_context = get_context(global_kernel_state.scheduler_cthread);
    assert(scheduler_context->state == CThread::State::RUNNING);
    assert(utl::is_link_detached(&scheduler_context->wait_link));

    scheduler_context->state = CThread::State::ZOMBIE;
    global_kernel_state.current_cthread.reset();
    enqueue_link(&global_kernel_state.zombie_list, &scheduler_context->wait_link);
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
    detach_link(&context->tasklist_link);
    --global_kernel_state.cthread_count;

    // Remove from Zombie List
    detach_link(&context->wait_link);
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
            utl::DLink* link = dequeue_link(&global_kernel_state.ready_list);
            CThread::Context* ctx = get_linked_cthread_context(link);
            CThread::Hdl task = CThread::Hdl::from_promise(*ctx);
            assert(ctx->state == CThread::State::READY);
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
                assert(ctx->state == CThread::State::IO_WAITING);

                // Move the target task from IO_WAITING to READY
                --global_kernel_state.iowaiting_cthread_count;
                ctx->state = CThread::State::READY;
                ++global_kernel_state.ready_cthread_count;
                enqueue_link(&global_kernel_state.ready_list, &ctx->wait_link);
                
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

            utl::DLink* zombie_node = dequeue_link(&global_kernel_state.zombie_list);
            CThread::Context& zombie_promise = *get_linked_cthread_context(zombie_node);
            assert(zombie_promise.state == CThread::State::ZOMBIE);

            // Remove from zombie list
            --global_kernel_state.zombie_cthread_count;
            detach_link(&zombie_promise.wait_link);

            // Remove from task list
            detach_link(&zombie_promise.tasklist_link);
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