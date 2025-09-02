#include "ak/runtime/runtime.hpp" // IWYU pragma: keep

#include <liburing.h>

// SuspendOp implmentation
// ----------------------------------------------------------------------------------------------------------------

AkCoroutineHandle AkSuspendTaskOp::await_suspend(AkCoroutineHandle current_task) const noexcept 
{
    AK_ASSERT(global_kernel_state.current_task);

    AkPromise* current_promise = &current_task.promise();

    if constexpr (AK_IS_DEBUG_MODE) {
        AK_ASSERT(global_kernel_state.current_task == current_task);
        AK_ASSERT(current_promise->state == AkCoroutineState::RUNNING);
        AK_ASSERT(ak_dlink_is_detached(&current_promise->wait_link));
        runtime_check_invariants();
    }

    // Move the current task from RUNNINIG to READY
    current_promise->state = AkCoroutineState::READY;
    ++global_kernel_state.ready_task_count;
    ak_dlink_enqueue(&global_kernel_state.ready_list, &current_promise->wait_link);
    global_kernel_state.current_task.reset();
    runtime_check_invariants();

    return runtime_schedule_next_thread();
}

// ResumeTaskOp implementation
// ----------------------------------------------------------------------------------------------------------------

AkCoroutineHandle AkResumeTaskOp::await_suspend(AkCoroutineHandle current_task_hdl) const noexcept
{
    AK_ASSERT(global_kernel_state.current_task == current_task_hdl);

    // Check the current Task
    AkPromise* current_promise = ak_get_promise(global_kernel_state.current_task);
    AK_ASSERT(ak_dlink_is_detached(&current_promise->wait_link));
    AK_ASSERT(current_promise->state == AkCoroutineState::RUNNING);
    runtime_check_invariants();

    // Suspend the current Task
    current_promise->state = AkCoroutineState::READY;
    ++global_kernel_state.ready_task_count;
    ak_dlink_enqueue(&global_kernel_state.ready_list, &current_promise->wait_link);
    global_kernel_state.current_task.reset();
    runtime_check_invariants();

    // Move the target task from READY to RUNNING
    AkPromise* promise = &hdl.promise();
    promise->state = AkCoroutineState::RUNNING;
    ak_dlink_detach(&promise->wait_link);
    --global_kernel_state.ready_task_count;
    global_kernel_state.current_task = hdl;
    runtime_check_invariants();

    AK_ASSERT(global_kernel_state.current_task);
    return hdl;
}

// JoinTaskOp implementation
// ----------------------------------------------------------------------------------------------------------------

AkCoroutineHandle AkJoinTaskOp::await_suspend(AkCoroutineHandle current_task_hdl) const noexcept
{
    AkPromise* current_task_ctx = &current_task_hdl.promise();

    // Check CurrentTask preconditions
    AK_ASSERT(current_task_ctx->state == AkCoroutineState::RUNNING);
    AK_ASSERT(ak_dlink_is_detached(&current_task_ctx->wait_link));
    AK_ASSERT(global_kernel_state.current_task == current_task_hdl);
    runtime_check_invariants();

    AkPromise* joined_task_ctx = &hdl.promise();                
    AkCoroutineState joined_task_state = joined_task_ctx->state;
    switch (joined_task_state) {
        case AkCoroutineState::READY:
        {

            // Move current Task from READY to WAITING
            current_task_ctx->state = AkCoroutineState::WAITING;
            ++global_kernel_state.waiting_task_count;
            ak_dlink_enqueue(&joined_task_ctx->awaiter_list, &current_task_ctx->wait_link); 
            global_kernel_state.current_task.reset();
            runtime_check_invariants();
            runtime_dump_task_count();

            // Move the joined TASK from READY to RUNNING
            joined_task_ctx->state = AkCoroutineState::RUNNING;
            ak_dlink_detach(&joined_task_ctx->wait_link);
            --global_kernel_state.ready_task_count;
            global_kernel_state.current_task = hdl;
            runtime_check_invariants();
            runtime_dump_task_count();
            return hdl;
        }

        case AkCoroutineState::IO_WAITING:
        case AkCoroutineState::WAITING:
        {
                // Move current Task from READY to WAITING
            current_task_ctx->state = AkCoroutineState::WAITING;
            ++global_kernel_state.waiting_task_count;
            ak_dlink_enqueue(&joined_task_ctx->awaiter_list, &current_task_ctx->wait_link); 
            global_kernel_state.current_task.reset();
            runtime_check_invariants();
            runtime_dump_task_count();

            // Move the Scheduler Task from READY to RUNNING
            AkPromise* sched_ctx = ak_get_promise(global_kernel_state.scheduler_task);
            AK_ASSERT(sched_ctx->state == AkCoroutineState::READY);
            sched_ctx->state = AkCoroutineState::RUNNING;
            ak_dlink_detach(&sched_ctx->wait_link);
            --global_kernel_state.ready_task_count;
            global_kernel_state.current_task = global_kernel_state.scheduler_task;
            runtime_check_invariants();
            runtime_dump_task_count();

            return global_kernel_state.scheduler_task;
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



