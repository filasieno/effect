#include "ak/runtime/runtime.hpp" // IWYU pragma: keep

#include <liburing.h>

namespace ak {

    // SuspendOp implmentation
    // ----------------------------------------------------------------------------------------------------------------

    CThread::Hdl op::Suspend::await_suspend(CThread::Hdl current_task) const noexcept {
        using namespace priv;

        AK_ASSERT(global_kernel_state.current_cthread);

        AkPromise* current_promise = &current_task.promise();

        if constexpr (AK_IS_DEBUG_MODE) {
            AK_ASSERT(global_kernel_state.current_cthread == current_task);
            AK_ASSERT(current_promise->state == AkCoroutineState::RUNNING);
            AK_ASSERT(ak_is_dlink_detached(&current_promise->wait_link));
            check_invariants();
        }

        // Move the current task from RUNNINIG to READY
        current_promise->state = AkCoroutineState::READY;
        ++global_kernel_state.ready_cthread_count;
        ak_enqueue_dlink(&global_kernel_state.ready_list, &current_promise->wait_link);
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
        AkPromise* current_promise = get_context(global_kernel_state.current_cthread);
        AK_ASSERT(ak_is_dlink_detached(&current_promise->wait_link));
        AK_ASSERT(current_promise->state == AkCoroutineState::RUNNING);
        check_invariants();

        // Suspend the current Task
        current_promise->state = AkCoroutineState::READY;
        ++global_kernel_state.ready_cthread_count;
        ak_enqueue_dlink(&global_kernel_state.ready_list, &current_promise->wait_link);
        global_kernel_state.current_cthread.reset();
        check_invariants();

        // Move the target task from READY to RUNNING
        AkPromise* promise = &hdl.promise();
        promise->state = AkCoroutineState::RUNNING;
        ak_detach_dlink(&promise->wait_link);
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

        AkPromise* current_task_ctx = &current_task_hdl.promise();

        // Check CurrentTask preconditions
        AK_ASSERT(current_task_ctx->state == AkCoroutineState::RUNNING);
        AK_ASSERT(ak_is_dlink_detached(&current_task_ctx->wait_link));
        AK_ASSERT(global_kernel_state.current_cthread == current_task_hdl);
        check_invariants();

        AkPromise* joined_task_ctx = &hdl.promise();                
        AkCoroutineState joined_task_state = joined_task_ctx->state;
        switch (joined_task_state) {
            case AkCoroutineState::READY:
            {

                // Move current Task from READY to WAITING
                current_task_ctx->state = AkCoroutineState::WAITING;
                ++global_kernel_state.waiting_cthread_count;
                ak_enqueue_dlink(&joined_task_ctx->awaiter_list, &current_task_ctx->wait_link); 
                global_kernel_state.current_cthread.reset();
                check_invariants();
                dump_task_count();

                // Move the joined TASK from READY to RUNNING
                joined_task_ctx->state = AkCoroutineState::RUNNING;
                ak_detach_dlink(&joined_task_ctx->wait_link);
                --global_kernel_state.ready_cthread_count;
                global_kernel_state.current_cthread = hdl;
                check_invariants();
                dump_task_count();
                return hdl;
            }

            case AkCoroutineState::IO_WAITING:
            case AkCoroutineState::WAITING:
            {
                 // Move current Task from READY to WAITING
                current_task_ctx->state = AkCoroutineState::WAITING;
                ++global_kernel_state.waiting_cthread_count;
                ak_enqueue_dlink(&joined_task_ctx->awaiter_list, &current_task_ctx->wait_link); 
                global_kernel_state.current_cthread.reset();
                check_invariants();
                dump_task_count();

                // Move the Scheduler Task from READY to RUNNING
                AkPromise* sched_ctx = get_context(global_kernel_state.scheduler_cthread);
                AK_ASSERT(sched_ctx->state == AkCoroutineState::READY);
                sched_ctx->state = AkCoroutineState::RUNNING;
                ak_detach_dlink(&sched_ctx->wait_link);
                --global_kernel_state.ready_cthread_count;
                global_kernel_state.current_cthread = global_kernel_state.scheduler_cthread;
                check_invariants();
                dump_task_count();

                return global_kernel_state.scheduler_cthread;
            }
            
            case AkCoroutineState::DELETING:
            case AkCoroutineState::ZOMBIE:
            {
                return current_task_hdl;
            }
            
            case AkCoroutineState::INVALID:
            case AkCoroutineState::CREATED:
            case AkCoroutineState::RUNNING:
            default:
            {
                // Illegal State
                std::abort();
            }
        }
    }

}


