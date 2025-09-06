#pragma once

#include "ak/runtime/runtime_api.hpp"
#include <cstdlib>


inline AkPromise::AkPromise() {
    ak_dlink_init(&tasklist_link);
    ak_dlink_init(&wait_link);  
    ak_dlink_init(&awaiter_list);
    state = AkCoroutineState::CREATED;
    prepared_io = 0;
    res = -1;

    // Check post-conditions
    AK_ASSERT(ak_dlink_is_detached(&tasklist_link));
    AK_ASSERT(ak_dlink_is_detached(&wait_link));
    AK_ASSERT(state == AkCoroutineState::CREATED);
    // check_invariants();
}

inline void* ak_alloc_mem(AkSize sz) noexcept 
{ 
    return alloc_table_try_malloc(&global_kernel_state.alloc_table, sz); 
}

inline void ak_free_mem(void* ptr, AkU32 side_coalesching) noexcept 
{ 
    alloc_table_free(&global_kernel_state.alloc_table, ptr, side_coalesching); 
}

inline int ak_defragment_mem(AkU64 millis_time_budget) noexcept { 
    return alloc_table_defrag(&global_kernel_state.alloc_table, millis_time_budget); 
}

inline AkPromise* runtime_get_linked_task_context(const ak_dlink* link) noexcept {
    unsigned long long promise_off = ((unsigned long long)link) - offsetof(AkPromise, wait_link);
    return reinterpret_cast<AkPromise*>(promise_off);
}

inline const char* ak_to_string(AkCoroutineState state) noexcept 
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

namespace ak 
{ 

    inline BootCThread BootCThread::Context::get_return_object_on_allocation_failure() noexcept 
    {
        std::abort(); /* unreachable */
    }

    namespace priv {
        
        // Scheduler operations
        // ----------------------------------------------------------------------------------------------------------------

        struct RunSchedulerOp {
            constexpr bool  await_ready() const noexcept { return false; }
            constexpr void  await_resume() const noexcept { }
            AkCoroutineHandle await_suspend(BootCThread::Hdl current_task_hdl) const noexcept;
        };
    
        struct TerminateSchedulerOp {
            constexpr bool await_ready() const noexcept  { return false; }
            constexpr void await_resume() const noexcept { }
            BootCThread::Hdl await_suspend(AkCoroutineHandle hdl) const noexcept;
        };

        constexpr RunSchedulerOp       run_scheduler() noexcept       { return {}; }
        constexpr TerminateSchedulerOp terminate_scheduler() noexcept { return {}; }
        void                         destroy_scheduler(AkTask hdl) noexcept;
        
        // Coroutine System Boot
        // ----------------------------------------------------------------------------------------------------------------

        template <typename... Args>
        BootCThread boot_main_proc(AkTask(*main_proc)(Args ...) noexcept, Args ... args) noexcept;
        
        template <typename... Args>
        AkTask scheduler_main_proc(AkTask(*main_proc)(Args ...) noexcept, Args... args) noexcept;

        template <typename... Args>
        BootCThread boot_main_proc(AkTask(*main_proc)(Args ...) noexcept, Args ... args) noexcept 
        {
            AkCoroutineHandle scheduler_hdl = ::ak::priv::scheduler_main_proc(main_proc, std::forward<Args>(args) ... );
            global_kernel_state.scheduler_task = scheduler_hdl;

            co_await ::ak::priv::run_scheduler();
            destroy_scheduler(scheduler_hdl);
            co_return;
        }

        template <typename... Args>
        AkTask scheduler_main_proc(AkTask(*main_proc)(Args ...) noexcept, Args... args) noexcept 
        {
            AkCoroutineHandle main_task = main_proc(args...);
            global_kernel_state.main_task = main_task;
            AK_ASSERT(!main_task.done());
            AK_ASSERT(ak_get_task_state(main_task) == AkCoroutineState::READY);

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
                if (global_kernel_state.ready_task_count > 0) {
                    ak_dlink* next_node = global_kernel_state.ready_list.prev;
                    AkPromise* next_promise = runtime_get_linked_task_context(next_node);
                    AkCoroutineHandle next_task = AkCoroutineHandle::from_promise(*next_promise);
                    AK_ASSERT(next_task != global_kernel_state.scheduler_task);
                    co_await AkResumeTaskOp(next_task);
                    AK_ASSERT(global_kernel_state.current_task);
                    continue;
                }

                // Zombie bashing
                while (global_kernel_state.zombie_task_count > 0) {
                    ak_dlink* zombie_link = ak_dlink_dequeue(&global_kernel_state.zombie_list);
                    AkPromise* ctx = runtime_get_linked_task_context(zombie_link);
                    AK_ASSERT(ctx->state == AkCoroutineState::ZOMBIE);

                    // Remove from zombie list
                    --global_kernel_state.zombie_task_count;
                    ak_dlink_detach(&ctx->wait_link);

                    // Remove from task list
                    ak_dlink_detach(&ctx->tasklist_link);
                    --global_kernel_state.task_count;

                    // Delete
                    ctx->state = AkCoroutineState::DELETING;
                    AkCoroutineHandle zombieTaskHdl = AkCoroutineHandle::from_promise(*ctx);
                    zombieTaskHdl.destroy();
                }

                bool waiting_cc = global_kernel_state.iowaiting_task_count;
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
                        --global_kernel_state.iowaiting_task_count;
                        ctx->state = AkCoroutineState::READY;
                        ++global_kernel_state.ready_task_count;
                        ak_dlink_enqueue(&global_kernel_state.ready_list, &ctx->wait_link);
                        
                        // Complete operation
                        ctx->res = cqe->res;
                        --ctx->prepared_io;
                        ++completed;
                    }
                    // Mark all as seen
                    io_uring_cq_advance(&global_kernel_state.io_uring_state, completed);
                }

                if (global_kernel_state.ready_task_count == 0 && global_kernel_state.iowaiting_task_count == 0) {
                    break;
                }
            }
            co_await terminate_scheduler();
            std::abort(); // Unreachable
        } 
    }
}

template <typename... Args>
int ak_run_main(AkTask(*main_proc)(Args ...) noexcept , Args... args) noexcept {
    auto boot_cthread = ak::priv::boot_main_proc(main_proc, std::forward<Args>(args) ...);
    global_kernel_state.boot_task = boot_cthread;
    boot_cthread.hdl.resume();
    return global_kernel_state.main_task_exit_code;
}

inline AkPromise*                ak_get_promise(AkTask ct) noexcept    { return &ct.hdl.promise(); }
inline AkPromise*                ak_get_promise() noexcept             { return &global_kernel_state.current_task.hdl.promise(); }
inline constexpr AkSuspendTaskOp ak_suspend_task() noexcept            { return {}; }
inline AkJoinTaskOp              ak_join_task(AkTask ct) noexcept      { return AkJoinTaskOp(ct); }
inline AkCoroutineState          ak_get_task_state(AkTask ct) noexcept { return ct.hdl.promise().state; }
inline bool                    ak_is_task_done(AkTask ct) noexcept   { return ct.hdl.done(); }
inline AkResumeTaskOp            ak_resume_task(AkTask ct) noexcept    { return AkResumeTaskOp(ct); }
inline bool                    ak_is_task_valid(AkTask ct) noexcept  { return ct.hdl.address() != nullptr; }
inline AkGetCurrentTaskOp        ak_get_task_promise_async() noexcept  { return {}; }
inline AkJoinTaskOp              operator co_await(AkTask ct) noexcept { return AkJoinTaskOp(ct); }


inline AkCoroutineHandle to_handle(AkPromise* promise) noexcept             { return AkCoroutineHandle::from_promise(*promise); }
inline AkTask AkPromise::get_return_object_on_allocation_failure() noexcept { return { }; }
inline AkTask AkPromise::get_return_object() noexcept                       { return { AkCoroutineHandle::from_promise(*this) }; }


