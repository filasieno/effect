#include "ak/runtime/runtime.hpp" // IWYU pragma: keep

#include <liburing.h>

namespace ak {

    // SuspendOp implmentation
    // ----------------------------------------------------------------------------------------------------------------

    CThread::Hdl op::Suspend::await_suspend(CThread::Hdl current_task) const noexcept {
        using namespace priv;

        AK_ASSERT(global_kernel_state.current_cthread);

        CThread::Context* current_promise = &current_task.promise();

        if constexpr (IS_DEBUG_MODE) {
            AK_ASSERT(global_kernel_state.current_cthread == current_task);
            AK_ASSERT(current_promise->state == CThread::State::RUNNING);
            AK_ASSERT(is_dlink_detached(&current_promise->wait_link));
            check_invariants();
        }

        // Move the current task from RUNNINIG to READY
        current_promise->state = CThread::State::READY;
        ++global_kernel_state.ready_cthread_count;
        enqueue_dlink(&global_kernel_state.ready_list, &current_promise->wait_link);
        global_kernel_state.current_cthread.reset();
        check_invariants();

        return schedule_next_thread();
    }
    
    // ResumeTaskOp implementation
    // ----------------------------------------------------------------------------------------------------------------

    CThread::Hdl op::ResumeCThread::await_suspend(CThread::Hdl current_task_hdl) const noexcept {
        using namespace priv;

        AK_ASSERT(global_kernel_state.current_cthread == current_task_hdl);

        // Check the current Task
        CThread::Context* current_promise = get_context(global_kernel_state.current_cthread);
        AK_ASSERT(is_dlink_detached(&current_promise->wait_link));
        AK_ASSERT(current_promise->state == CThread::State::RUNNING);
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

        AK_ASSERT(global_kernel_state.current_cthread);
        return hdl;
    }

    // JoinTaskOp implementation
    // ----------------------------------------------------------------------------------------------------------------

    CThread::Hdl op::JoinCThread::await_suspend(CThread::Hdl current_task_hdl) const noexcept
    {
        using namespace priv;

        CThread::Context* current_task_ctx = &current_task_hdl.promise();

        // Check CurrentTask preconditions
        AK_ASSERT(current_task_ctx->state == CThread::State::RUNNING);
        AK_ASSERT(is_dlink_detached(&current_task_ctx->wait_link));
        AK_ASSERT(global_kernel_state.current_cthread == current_task_hdl);
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
                AK_ASSERT(sched_ctx->state == CThread::State::READY);
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
                std::abort();
            }
        }
    }

}


