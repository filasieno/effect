#pragma once

#include "ak/api_priv.hpp"

namespace ak {

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

    // TaskContext implementation 
    // ----------------------------------------------------------------------------------------------------------------

    inline Void CThread::Context::InitialSuspendTaskOp::await_suspend(CThread::Hdl hdl) const noexcept {
        using namespace priv;
        using namespace utl;

        CThread::Context* promise = &hdl.promise();

        // Check initial preconditions
        assert(promise->state == CThread::State::CREATED);
        assert(is_dlink_detached(&promise->wait_link));
        check_invariants();

        // Add task to the kernel
        ++global_kernel_state.cthread_count;
        enqueue_dlink(&global_kernel_state.cthread_list, &promise->tasklist_link);

        ++global_kernel_state.ready_cthread_count;
        enqueue_dlink(&global_kernel_state.ready_list, &promise->wait_link);
        promise->state = CThread::State::READY;

        // Check post-conditions
        assert(promise->state == CThread::State::READY);
        assert(!is_dlink_detached(&promise->wait_link));
        check_invariants();
        priv::dump_task_count();
    }

    inline CThread::Hdl CThread::Context::FinalSuspendTaskOp::await_suspend(CThread::Hdl hdl) const noexcept {
        using namespace priv;
        using namespace utl;

        // Check preconditions
        CThread::Context* ctx = &hdl.promise();
        assert(global_kernel_state.current_cthread == hdl);
        assert(ctx->state == CThread::State::RUNNING);
        assert(is_dlink_detached(&ctx->wait_link));
        check_invariants();

        // Move the current task from RUNNING to ZOMBIE
        ctx->state = CThread::State::ZOMBIE;
        ++global_kernel_state.zombie_cthread_count;
        enqueue_dlink(&global_kernel_state.zombie_list, &ctx->wait_link);
        global_kernel_state.current_cthread = CThread();
        check_invariants();

        return schedule_cthread();
    }

    // TaskContext ctor/dtor definitions
    inline CThread::Context::~Context() {
        using namespace priv;
        assert(state == CThread::State::DELETING);
        assert(is_dlink_detached(&tasklist_link));
        assert(is_dlink_detached(&wait_link));
        dump_task_count();
        check_invariants();
    }

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
        assert(is_dlink_detached(&tasklist_link));
        assert(is_dlink_detached(&wait_link));
        assert(state == CThread::State::CREATED);
        check_invariants();
    }

    inline Void* CThread::Context::operator new(std::size_t n) noexcept {
        Void* mem = try_alloc_mem(n);
        if (!mem) return nullptr;
        return mem;
    }

    inline Void CThread::Context::operator delete(Void* ptr, std::size_t sz) {
        (Void)sz;
        free_mem(ptr);
    }

    inline Void CThread::Context::return_value(int value) noexcept {
        using namespace priv;

        check_invariants();

        CThread::Context* current_context = get_context(global_kernel_state.current_cthread);
        current_context->res = value;
        if (global_kernel_state.current_cthread == global_kernel_state.main_cthread) {
            std::print("MainTask done; returning: {}\n", value);
            global_kernel_state.main_cthread_exit_code = value;
        }

        // Wake up all tasks waiting for this task
        if (is_dlink_detached(&awaiter_list)) {
            return;
        }

        do {
            utl::DLink* next = dequeue_dlink(&awaiter_list);
            CThread::Context* ctx = get_linked_cthread_context(next);
            dump_task_count();
            assert(ctx->state == CThread::State::WAITING);
            --global_kernel_state.waiting_cthread_count;
            ctx->state = CThread::State::READY;
            enqueue_dlink(&global_kernel_state.ready_list, &ctx->wait_link);
            ++global_kernel_state.ready_cthread_count;
            dump_task_count();

        } while (!is_dlink_detached(&awaiter_list));

    }


    // SuspendOp implmentation
    // ----------------------------------------------------------------------------------------------------------------

    inline CThread::Hdl op::Suspend::await_suspend(CThread::Hdl current_task) const noexcept {
        using namespace priv;
        using namespace utl;

        assert(global_kernel_state.current_cthread);

        CThread::Context* current_promise = &current_task.promise();

        if constexpr (IS_DEBUG_MODE) {
            assert(global_kernel_state.current_cthread == current_task);
            assert(current_promise->state == CThread::State::RUNNING);
            assert(is_dlink_detached(&current_promise->wait_link));
            check_invariants();
        }

        // Move the current task from RUNNINIG to READY
        current_promise->state = CThread::State::READY;
        ++global_kernel_state.ready_cthread_count;
        enqueue_dlink(&global_kernel_state.ready_list, &current_promise->wait_link);
        global_kernel_state.current_cthread.reset();
        check_invariants();

        return schedule_cthread();
    }
    
    // ResumeTaskOp implementation
    // ----------------------------------------------------------------------------------------------------------------

    inline CThread::Hdl op::ResumeCThread::await_suspend(CThread::Hdl current_task_hdl) const noexcept {
        using namespace priv;
        using namespace utl;

        assert(global_kernel_state.current_cthread == current_task_hdl);

        // Check the current Task
        CThread::Context* current_promise = get_context(global_kernel_state.current_cthread);
        assert(is_dlink_detached(&current_promise->wait_link));
        assert(current_promise->state == CThread::State::RUNNING);
        check_invariants();

        // Suspend the current Task
        current_promise->state = CThread::State::READY;
        ++global_kernel_state.ready_cthread_count;
        enqueue_dlink(&global_kernel_state.ready_list, &current_promise->wait_link);
        global_kernel_state.current_cthread.reset();
        check_invariants();

        // Move the target task from READY to RUNNING
        CThread::Context* promise = &hdl.promise();
        promise->state = CThread::State::RUNNING;
        detach_dlink(&promise->wait_link);
        --global_kernel_state.ready_cthread_count;
        global_kernel_state.current_cthread = hdl;
        check_invariants();

        assert(global_kernel_state.current_cthread);
        return hdl;
    }

    // JoinTaskOp implementation
    // ----------------------------------------------------------------------------------------------------------------

    inline CThread::Hdl op::JoinCThread::await_suspend(CThread::Hdl current_task_hdl) const noexcept
    {
        using namespace priv;
        using namespace utl;

        CThread::Context* current_task_ctx = &current_task_hdl.promise();

        // Check CurrentTask preconditions
        assert(current_task_ctx->state == CThread::State::RUNNING);
        assert(is_dlink_detached(&current_task_ctx->wait_link));
        assert(global_kernel_state.current_cthread == current_task_hdl);
        check_invariants();

        CThread::Context* joined_task_ctx = &hdl.promise();                
        CThread::State joined_task_state = joined_task_ctx->state;
        switch (joined_task_state) {
            case CThread::State::READY:
            {

                // Move current Task from READY to WAITING
                current_task_ctx->state = CThread::State::WAITING;
                ++global_kernel_state.waiting_cthread_count;
                enqueue_dlink(&joined_task_ctx->awaiter_list, &current_task_ctx->wait_link); 
                global_kernel_state.current_cthread.reset();
                check_invariants();
                dump_task_count();

                // Move the joined TASK from READY to RUNNING
                joined_task_ctx->state = CThread::State::RUNNING;
                detach_dlink(&joined_task_ctx->wait_link);
                --global_kernel_state.ready_cthread_count;
                global_kernel_state.current_cthread = hdl;
                check_invariants();
                dump_task_count();
                return hdl;
            }

            case CThread::State::IO_WAITING:
            case CThread::State::WAITING:
            {
                 // Move current Task from READY to WAITING
                current_task_ctx->state = CThread::State::WAITING;
                ++global_kernel_state.waiting_cthread_count;
                enqueue_dlink(&joined_task_ctx->awaiter_list, &current_task_ctx->wait_link); 
                global_kernel_state.current_cthread.reset();
                check_invariants();
                dump_task_count();

                // Move the Scheduler Task from READY to RUNNING
                CThread::Context* sched_ctx = get_context(global_kernel_state.scheduler_cthread);
                assert(sched_ctx->state == CThread::State::READY);
                sched_ctx->state = CThread::State::RUNNING;
                detach_dlink(&sched_ctx->wait_link);
                --global_kernel_state.ready_cthread_count;
                global_kernel_state.current_cthread = global_kernel_state.scheduler_cthread;
                check_invariants();
                dump_task_count();

                return global_kernel_state.scheduler_cthread;
            }
            
            case CThread::State::DELETING:
            case CThread::State::ZOMBIE:
            {
                return current_task_hdl;
            }
            
            case CThread::State::INVALID:
            case CThread::State::CREATED:
            case CThread::State::RUNNING:
            default:
            {
                // Illegal State
                abort();
            }
        }
    }

    // Task API Implementation
    // ----------------------------------------------------------------------------------------------------------------

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

}

// Private API Implementation
// ----------------------------------------------------------------------------------------------------------------

namespace ak { namespace priv {

    inline CThread::Context* get_linked_cthread_context(const utl::DLink* link) noexcept {
        unsigned long long promise_off = ((unsigned long long)link) - offsetof(CThread::Context, wait_link);
        return reinterpret_cast<CThread::Context*>(promise_off);
    }
}} // namespace ak::priv
