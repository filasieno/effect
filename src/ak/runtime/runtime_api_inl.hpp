#pragma once

// Public inline API implementation
// --------------------------------

#include "ak/runtime/runtime_api.hpp"
#include "ak/runtime/runtime_api_priv.hpp"

namespace ak { 

    template <typename... Args>
    inline CThread::Context::Context(Args&&... ) {
        using namespace priv;

        init_dlink(&tasklist_link);
        init_dlink(&wait_link);  
        init_dlink(&awaiter_list);
        state = CThread::State::CREATED;
        prepared_io = 0;
        res = -1;

        // Check post-conditions
        AK_ASSERT(is_dlink_detached(&tasklist_link));
        AK_ASSERT(is_dlink_detached(&wait_link));
        AK_ASSERT(state == CThread::State::CREATED);
        // check_invariants();
    }

    inline const Char* to_string(CThread::State state) noexcept 
    {
        switch (state) {
            case CThread::State::INVALID:    return "INVALID";
            case CThread::State::CREATED:    return "CREATED";
            case CThread::State::READY:      return "READY";
            case CThread::State::RUNNING:    return "RUNNING";
            case CThread::State::IO_WAITING: return "IO_WAITING";
            case CThread::State::WAITING:    return "WAITING";
            case CThread::State::ZOMBIE:     return "ZOMBIE";
            case CThread::State::DELETING:   return "DELETING";
            default: return nullptr;
        }
    }

    inline Bool is_valid(CThread ct) noexcept { return ct.hdl.address() != nullptr; }

    inline CThread::Context* get_context(CThread ct) noexcept { return &ct.hdl.promise(); }

    inline CThread::Context* get_context() noexcept { return &global_kernel_state.current_cthread.hdl.promise(); }

    inline constexpr op::GetCurrentTask get_cthread_context_async() noexcept { return {}; }

    inline constexpr op::Suspend suspend() noexcept { return {}; }

    inline op::JoinCThread join(CThread ct) noexcept { return op::JoinCThread(ct); }

    inline op::JoinCThread operator co_await(CThread ct) noexcept { return op::JoinCThread(ct); }

    inline CThread::State get_state(CThread ct) noexcept { return ct.hdl.promise().state; }

    inline Bool is_done(CThread ct) noexcept { return ct.hdl.done(); }

    inline op::ResumeCThread resume(CThread ct) noexcept { return op::ResumeCThread(ct); }

    namespace priv {
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
 
        // Scheduler task routines
        constexpr RunSchedulerOp       run_scheduler() noexcept;
        constexpr TerminateSchedulerOp terminate_scheduler() noexcept;
        constexpr Void                 destroy_scheduler(CThread hdl) noexcept;
        
        template <typename... Args>
        inline BootCThread boot_main_proc(CThread(*main_proc)(Args ...) noexcept, Args ... args) noexcept {
            CThread::Hdl scheduler_hdl = scheduler_main_proc(main_proc, std::forward<Args>(args) ... );
            global_kernel_state.scheduler_cthread = scheduler_hdl;

            co_await run_scheduler();
            destroy_scheduler(scheduler_hdl);
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

    // Make the main entry template visible to all translation units
    template <typename... Args>
    inline int run_main(CThread(*main_proc)(Args ...) noexcept , Args... args) noexcept {
        using namespace priv;
        auto boot_cthread = ::ak::priv::boot_main_proc(main_proc, std::forward<Args>(args) ...);
        global_kernel_state.boot_cthread = boot_cthread;
        boot_cthread.hdl.resume();
        return global_kernel_state.main_cthread_exit_code;
    }
}


