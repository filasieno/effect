#pragma once

// Public inline API implementation
// --------------------------------

#include "ak/runtime/runtime_api.hpp"
#include <cstdlib>


inline AkPromise::AkPromise() {

    ak_init_dlink(&tasklist_link);
    ak_init_dlink(&wait_link);  
    ak_init_dlink(&awaiter_list);
    state = AkCoroutineState::CREATED;
    prepared_io = 0;
    res = -1;

    // Check post-conditions
    AK_ASSERT(ak_is_dlink_detached(&tasklist_link));
    AK_ASSERT(ak_is_dlink_detached(&wait_link));
    AK_ASSERT(state == AkCoroutineState::CREATED);
    // check_invariants();
}

namespace ak { 

    // Inline Context
    // ----------------------------------------------------------------------------------------------------------------

    inline BootCThread BootCThread::Context::get_return_object_on_allocation_failure() noexcept 
    {
        std::abort(); /* unreachable */
    }


    inline const AkChar* to_string(AkCoroutineState state) noexcept 
    {
        switch (state) {
            case AkCoroutineState::INVALID:    return "INVALID";
            case AkCoroutineState::CREATED:    return "CREATED";
            case AkCoroutineState::READY:      return "READY";
            case AkCoroutineState::RUNNING:    return "RUNNING";
            case AkCoroutineState::IO_WAITING: return "IO_WAITING";
            case AkCoroutineState::WAITING:    return "WAITING";
            case AkCoroutineState::ZOMBIE:     return "ZOMBIE";
            case AkCoroutineState::DELETING:   return "DELETING";
            default: return nullptr;
        }
    }

    // Inline Public API Implementation
    // ----------------------------------------------------------------------------------------------------------------

    inline AkBool is_valid(CThread ct) noexcept { return ct.hdl.address() != nullptr; }

    inline AkPromise* get_context(CThread ct) noexcept { return &ct.hdl.promise(); }

    inline AkPromise* get_context() noexcept { return &global_kernel_state.current_cthread.hdl.promise(); }

    inline constexpr op::GetCurrentTask get_cthread_context_async() noexcept { return {}; }

    inline constexpr op::Suspend suspend() noexcept { return {}; }

    inline op::JoinCThread join(CThread ct) noexcept { return op::JoinCThread(ct); }

    inline op::JoinCThread operator co_await(CThread ct) noexcept { return op::JoinCThread(ct); }

    inline AkCoroutineState get_state(CThread ct) noexcept { return ct.hdl.promise().state; }

    inline AkBool is_done(CThread ct) noexcept { return ct.hdl.done(); }

    inline op::ResumeCThread resume(CThread ct) noexcept { return op::ResumeCThread(ct); }

    inline AkVoid* try_alloc_mem(AkSize sz) noexcept { return priv::try_alloc_table_malloc(&global_kernel_state.alloc_table, sz); }

    inline AkVoid free_mem(AkVoid* ptr, AkU32 side_coalesching) noexcept { priv::alloc_table_free(&global_kernel_state.alloc_table, ptr, side_coalesching); }

    inline AkI32 defragment_mem(AkU64 millis_time_budget) noexcept { return priv::defrag_alloc_table_mem(&global_kernel_state.alloc_table, millis_time_budget); }

    // Boot operations
    // ----------------------------------------------------------------------------------------------------------------

    namespace priv {
        
        inline AkPromise* get_linked_cthread_context(const AkDLink* link) noexcept {
            unsigned long long promise_off = ((unsigned long long)link) - offsetof(AkPromise, wait_link);
            return reinterpret_cast<AkPromise*>(promise_off);
        }

        // Scheduler operations
        // ----------------------------------------------------------------------------------------------------------------

        struct RunSchedulerOp {
            constexpr AkBool await_ready() const noexcept { return false; }
            constexpr AkVoid await_resume() const noexcept { }
            CThread::Hdl   await_suspend(BootCThread::Hdl current_task_hdl) const noexcept;
        };
    
        struct TerminateSchedulerOp {
            constexpr AkBool   await_ready() const noexcept { return false; }
            constexpr AkVoid   await_resume() const noexcept { }
            BootCThread::Hdl await_suspend(CThread::Hdl hdl) const noexcept;
        };

        constexpr RunSchedulerOp       run_scheduler() noexcept       { return {}; }
        
        constexpr TerminateSchedulerOp terminate_scheduler() noexcept { return {}; }
        
        AkVoid                           destroy_scheduler(CThread hdl) noexcept;
        
        // Coroutine System Boot
        // ----------------------------------------------------------------------------------------------------------------

        template <typename... Args>
        BootCThread boot_main_proc(CThread(*main_proc)(Args ...) noexcept, Args ... args) noexcept;
        
        template <typename... Args>
        CThread scheduler_main_proc(CThread(*main_proc)(Args ...) noexcept, Args... args) noexcept;

        template <typename... Args>
        BootCThread boot_main_proc(CThread(*main_proc)(Args ...) noexcept, Args ... args) noexcept 
        {
            CThread::Hdl scheduler_hdl = ::ak::priv::scheduler_main_proc(main_proc, std::forward<Args>(args) ... );
            global_kernel_state.scheduler_cthread = scheduler_hdl;

            co_await ::ak::priv::run_scheduler();
            destroy_scheduler(scheduler_hdl);
            co_return;
        }

        template <typename... Args>
        CThread scheduler_main_proc(CThread(*main_proc)(Args ...) noexcept, Args... args) noexcept 
        {
            CThread::Hdl main_task = main_proc(args...);
            global_kernel_state.main_cthread = main_task;
            AK_ASSERT(!main_task.done());
            AK_ASSERT(get_state(main_task) == AkCoroutineState::READY);

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
                    AkDLink* next_node = global_kernel_state.ready_list.prev;
                    AkPromise* next_promise = get_linked_cthread_context(next_node);
                    CThread::Hdl next_task = CThread::Hdl::from_promise(*next_promise);
                    AK_ASSERT(next_task != global_kernel_state.scheduler_cthread);
                    co_await op::ResumeCThread(next_task);
                    AK_ASSERT(global_kernel_state.current_cthread);
                    continue;
                }

                // Zombie bashing
                while (global_kernel_state.zombie_cthread_count > 0) {
                    AkDLink* zombie_link = ak_dequeue_dlink(&global_kernel_state.zombie_list);
                    AkPromise* ctx = get_linked_cthread_context(zombie_link);
                    AK_ASSERT(ctx->state == AkCoroutineState::ZOMBIE);

                    // Remove from zombie list
                    --global_kernel_state.zombie_cthread_count;
                    ak_detach_dlink(&ctx->wait_link);

                    // Remove from task list
                    ak_detach_dlink(&ctx->tasklist_link);
                    --global_kernel_state.cthread_count;

                    // Delete
                    ctx->state = AkCoroutineState::DELETING;
                    CThread::Hdl zombieTaskHdl = CThread::Hdl::from_promise(*ctx);
                    zombieTaskHdl.destroy();
                }

                AkBool waiting_cc = global_kernel_state.iowaiting_cthread_count;
                if (waiting_cc) {
                    // Process all available completions
                    struct io_uring_cqe *cqe;
                    unsigned head;
                    unsigned completed = 0;
                    io_uring_for_each_cqe(&global_kernel_state.io_uring_state, head, cqe) {
                        // Return Result to the target Awaitable 
                        AkPromise* ctx = (AkPromise*) io_uring_cqe_get_data(cqe);
                        AK_ASSERT(ctx->state == AkCoroutineState::IO_WAITING);

                        // Move the target task from IO_WAITING to READY
                        --global_kernel_state.iowaiting_cthread_count;
                        ctx->state = AkCoroutineState::READY;
                        ++global_kernel_state.ready_cthread_count;
                        ak_enqueue_dlink(&global_kernel_state.ready_list, &ctx->wait_link);
                        
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
    int run_main(CThread(*main_proc)(Args ...) noexcept , Args... args) noexcept {
        auto boot_cthread = priv::boot_main_proc(main_proc, std::forward<Args>(args) ...);
        global_kernel_state.boot_cthread = boot_cthread;
        boot_cthread.hdl.resume();
        return global_kernel_state.main_cthread_exit_code;
    }
}


