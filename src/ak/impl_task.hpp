#pragma once

#include "ak/api_priv.hpp"

namespace ak {

    inline const Char* to_string(TaskState state) noexcept 
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

    // TaskContext implementation 
    // ----------------------------------------------------------------------------------------------------------------

    inline Void CThreadContext::InitialSuspendTaskOp::await_suspend(CThreadCtxHdl hdl) const noexcept {
        using namespace priv;
        using namespace utl;

        CThreadContext* promise = &hdl.promise();

        // Check initial preconditions
        assert(promise->state == TaskState::CREATED);
        assert(is_link_detached(&promise->wait_link));
        CheckInvariants();

        // Add task to the kernel
        ++gKernel.task_count;
        enqueue_link(&gKernel.task_list, &promise->tasklist_link);

        ++gKernel.ready_count;
        enqueue_link(&gKernel.ready_list, &promise->wait_link);
        promise->state = TaskState::READY;

        // Check post-conditions
        assert(promise->state == TaskState::READY);
        assert(!is_link_detached(&promise->wait_link));
        CheckInvariants();
        priv::DebugTaskCount();
    }

    inline CThreadCtxHdl CThreadContext::FinalSuspendTaskOp::await_suspend(CThreadCtxHdl hdl) const noexcept {
        using namespace priv;
        using namespace utl;

        // Check preconditions
        CThreadContext* ctx = &hdl.promise();
        assert(gKernel.current_ctx_hdl == hdl);
        assert(ctx->state == TaskState::RUNNING);
        assert(is_link_detached(&ctx->wait_link));
        CheckInvariants();

        // Move the current task from RUNNING to ZOMBIE
        ctx->state = TaskState::ZOMBIE;
        ++gKernel.zombie_count;
        enqueue_link(&gKernel.zombie_list, &ctx->wait_link);
        clear(&gKernel.current_ctx_hdl);
        CheckInvariants();

        return ScheduleNextTask();
    }

    // TaskContext ctor/dtor definitions
    inline CThreadContext::~CThreadContext() {
        using namespace priv;
        assert(state == TaskState::DELETING);
        assert(is_link_detached(&tasklist_link));
        assert(is_link_detached(&wait_link));
        DebugTaskCount();
        CheckInvariants();
    }

    template <typename... Args>
    inline CThreadContext::CThreadContext(Args&&... ) {
        using namespace priv;

        init_link(&tasklist_link);
        init_link(&wait_link);
        init_link(&awaiter_list);
        state = TaskState::CREATED;
        prepared_io = 0;
        res = -1;

        // Check post-conditions
        assert(is_link_detached(&tasklist_link));
        assert(is_link_detached(&wait_link));
        assert(state == TaskState::CREATED);
        CheckInvariants();
    }

    inline Void* CThreadContext::operator new(std::size_t n) noexcept {
        Void* mem = try_alloc_mem(n);
        if (!mem) return nullptr;
        return mem;
    }

    inline Void CThreadContext::operator delete(Void* ptr, std::size_t sz) {
        (Void)sz;
        free_mem(ptr);
    }

    inline Void CThreadContext::return_value(int value) noexcept {
        using namespace priv;

        CheckInvariants();

        CThreadCtxHdl currentTaskHdl = gKernel.current_ctx_hdl;
        CThreadContext* ctx = &currentTaskHdl.promise();
        ctx->res = value;
        if (currentTaskHdl == gKernel.co_main_ctx_hdl) {
            std::print("MainTask done; returning: {}\n", value);
            gKernel.mainTaskReturnValue = value;
        }

        // Wake up all tasks waiting for this task
        if (is_link_detached(&awaiter_list)) {
            return;
        }

        do {
            utl::DLink* next = dequeue_link(&awaiter_list);
            CThreadContext* ctx = get_linked_context(next);
            DebugTaskCount();
            assert(ctx->state == TaskState::WAITING);
            --gKernel.waiting_count;
            ctx->state = TaskState::READY;
            enqueue_link(&gKernel.ready_list, &ctx->wait_link);
            ++gKernel.ready_count;
            DebugTaskCount();

        } while (!is_link_detached(&awaiter_list));

    }


    // SuspendOp implmentation
    // ----------------------------------------------------------------------------------------------------------------

    inline CThreadCtxHdl SuspendOp::await_suspend(CThreadCtxHdl currentTask) const noexcept {
        using namespace priv;
        using namespace utl;

        assert(gKernel.current_ctx_hdl);

        CThreadContext* currentPromise = &currentTask.promise();

        if constexpr (IS_DEBUG_MODE) {
            assert(gKernel.current_ctx_hdl == currentTask);
            assert(currentPromise->state == TaskState::RUNNING);
            assert(is_link_detached(&currentPromise->wait_link));
            CheckInvariants();
        }

        // Move the current task from RUNNINIG to READY
        currentPromise->state = TaskState::READY;
        ++gKernel.ready_count;
        enqueue_link(&gKernel.ready_list, &currentPromise->wait_link);
        clear(&gKernel.current_ctx_hdl);
        CheckInvariants();

        return ScheduleNextTask();
    }
    
    // ResumeTaskOp implementation
    // ----------------------------------------------------------------------------------------------------------------

    inline CThreadCtxHdl ResumeTaskOp::await_suspend(CThreadCtxHdl currentTaskHdl) const noexcept {
        using namespace priv;
        using namespace utl;

        assert(gKernel.current_ctx_hdl == currentTaskHdl);

        // Check the current Task
        CThreadContext* currentPromise = &gKernel.current_ctx_hdl.promise();
        assert(is_link_detached(&currentPromise->wait_link));
        assert(currentPromise->state == TaskState::RUNNING);
        CheckInvariants();

        // Suspend the current Task
        currentPromise->state = TaskState::READY;
        ++gKernel.ready_count;
        enqueue_link(&gKernel.ready_list, &currentPromise->wait_link);
        clear(&gKernel.current_ctx_hdl);
        CheckInvariants();

        // Move the target task from READY to RUNNING
        CThreadContext* promise = &hdl.promise();
        promise->state = TaskState::RUNNING;
        detach_link(&promise->wait_link);
        --gKernel.ready_count;
        gKernel.current_ctx_hdl = hdl;
        CheckInvariants();

        assert(gKernel.current_ctx_hdl);
        return hdl;
    }

    // JoinTaskOp implementation
    // ----------------------------------------------------------------------------------------------------------------

    inline CThreadCtxHdl JoinTaskOp::await_suspend(CThreadCtxHdl currentTaskHdl) const noexcept
    {
        using namespace priv;
        using namespace utl;

        CThreadContext* currentTaskCtx = &currentTaskHdl.promise();

        // Check CurrentTask preconditions
        assert(currentTaskCtx->state == TaskState::RUNNING);
        assert(is_link_detached(&currentTaskCtx->wait_link));
        assert(gKernel.current_ctx_hdl == currentTaskHdl);
        CheckInvariants();

        CThreadContext* joinedTaskCtx = &joinedTaskHdl.promise();                
        TaskState joinedTaskState = joinedTaskCtx->state;
        switch (joinedTaskState) {
            case TaskState::READY:
            {

                // Move current Task from READY to WAITING
                currentTaskCtx->state = TaskState::WAITING;
                ++gKernel.waiting_count;
                enqueue_link(&joinedTaskCtx->awaiter_list, &currentTaskCtx->wait_link); 
                clear(&gKernel.current_ctx_hdl);
                CheckInvariants();
                DebugTaskCount();

                // Move the joined TASK from READY to RUNNING
                joinedTaskCtx->state = TaskState::RUNNING;
                detach_link(&joinedTaskCtx->wait_link);
                --gKernel.ready_count;
                gKernel.current_ctx_hdl = joinedTaskHdl;
                CheckInvariants();
                DebugTaskCount();
                return joinedTaskHdl;
            }

            case TaskState::IO_WAITING:
            case TaskState::WAITING:
            {
                 // Move current Task from READY to WAITING
                currentTaskCtx->state = TaskState::WAITING;
                ++gKernel.waiting_count;
                enqueue_link(&joinedTaskCtx->awaiter_list, &currentTaskCtx->wait_link); 
                clear(&gKernel.current_ctx_hdl);
                CheckInvariants();
                DebugTaskCount();

                // Move the Scheduler Task from READY to RUNNING
                CThreadContext* schedCtx = &gKernel.scheduler_ctx_hdl.promise();
                assert(schedCtx->state == TaskState::READY);
                schedCtx->state = TaskState::RUNNING;
                detach_link(&schedCtx->wait_link);
                --gKernel.ready_count;
                gKernel.current_ctx_hdl = gKernel.scheduler_ctx_hdl;
                CheckInvariants();
                DebugTaskCount();

                return gKernel.scheduler_ctx_hdl;
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
                abort();
            }
        }
    }

    // Task API Implementation
    // ----------------------------------------------------------------------------------------------------------------

    inline Void clear(CThreadCtxHdl* hdl) noexcept { *hdl = CThreadCtxHdl{}; }

    inline Bool is_valid(CThreadCtxHdl hdl) noexcept { return hdl.address() != nullptr; }

    inline CThreadContext* get_task_context(CThreadCtxHdl hdl) noexcept { return &hdl.promise(); }

    inline CThreadContext* get_task_context() noexcept { return &gKernel.current_ctx_hdl.promise(); }

    inline constexpr GetCurrentTaskOp get_current_context() noexcept { return {}; }

    inline constexpr SuspendOp suspend() noexcept { return {}; }

    inline JoinTaskOp join(CThreadCtxHdl hdl) noexcept { return JoinTaskOp(hdl); }

    inline JoinTaskOp operator co_await(CThreadCtxHdl hdl) noexcept { return JoinTaskOp(hdl); }

    inline TaskState get_task_state(CThreadCtxHdl hdl) noexcept { return hdl.promise().state; }

    inline Bool is_done(CThreadCtxHdl hdl) noexcept { return hdl.done(); }

    inline ResumeTaskOp resume(CThreadCtxHdl hdl) noexcept { return ResumeTaskOp(hdl); }

}

// Private API Implementation
// ----------------------------------------------------------------------------------------------------------------

namespace ak { namespace priv {

    inline CThreadContext* get_linked_context(const utl::DLink* link) noexcept {
        unsigned long long promise_off = ((unsigned long long)link) - offsetof(CThreadContext, wait_link);
        return reinterpret_cast<CThreadContext*>(promise_off);
    }
}} // namespace ak::priv
