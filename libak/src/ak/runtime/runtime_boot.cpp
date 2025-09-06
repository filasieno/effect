#include "ak/runtime/runtime.hpp" // IWYU pragma: keep

#include <print>

namespace ak { 
   
    void* BootCThread::Context::operator new(std::size_t n) noexcept {
        AK_ASSERT(n <= sizeof(global_kernel_state.boot_task_frame_buffer));
        return (void*)global_kernel_state.boot_task_frame_buffer;
    }    

}

// Kernel boot implementation 
// ================================================================================================================

namespace ak::priv {

    // RunSchedulerTaskOp
    // ----------------------------------------------------------------------------------------------------------------

    AkCoroutineHandle RunSchedulerOp::await_suspend(BootCThread::Hdl current_task_hdl) const noexcept {
        using namespace priv;

        (void)current_task_hdl;
        AkPromise* scheduler_ctx = ak_get_promise(global_kernel_state.scheduler_task);

        // Check expected state post scheduler construction

        AK_ASSERT(global_kernel_state.task_count == 1);
        AK_ASSERT(global_kernel_state.ready_task_count == 1);
        AK_ASSERT(scheduler_ctx->state == AkCoroutineState::READY);
        AK_ASSERT(!ak_dlink_is_detached(&scheduler_ctx->wait_link));
        AK_ASSERT(global_kernel_state.current_task == AkCoroutineHandle());

        // Setup SchedulerTask for execution (from READY -> RUNNING)
        global_kernel_state.current_task = global_kernel_state.scheduler_task;
        scheduler_ctx->state = AkCoroutineState::RUNNING;
        ak_dlink_detach(&scheduler_ctx->wait_link);
        --global_kernel_state.ready_task_count;

        // Check expected state post task system bootstrap
        //check_invariants();
        return global_kernel_state.scheduler_task;
    }

    // TerminateSchedulerOp
    // ----------------------------------------------------------------------------------------------------------------

    BootCThread::Hdl TerminateSchedulerOp::await_suspend(AkCoroutineHandle hdl) const noexcept {
        using namespace priv;

        AK_ASSERT(global_kernel_state.current_task == global_kernel_state.scheduler_task);
        AK_ASSERT(global_kernel_state.current_task == hdl);

        auto* scheduler_context = ak_get_promise(global_kernel_state.scheduler_task);
        AK_ASSERT(scheduler_context->state == AkCoroutineState::RUNNING);
        AK_ASSERT(ak_dlink_is_detached(&scheduler_context->wait_link));

        scheduler_context->state = AkCoroutineState::ZOMBIE;
        global_kernel_state.current_task.reset();
        ak_dlink_enqueue(&global_kernel_state.zombie_list, &scheduler_context->wait_link);
        ++global_kernel_state.zombie_task_count;

        return global_kernel_state.boot_task;
    }

    // Boot implementation
    // ----------------------------------------------------------------------------------------------------------------

    void destroy_scheduler(AkTask ct) noexcept {
        using namespace priv;
        auto* context = ak_get_promise(ct);

        // Remove from Task list
        ak_dlink_detach(&context->tasklist_link);
        --global_kernel_state.task_count;

        // Remove from Zombie List
        ak_dlink_detach(&context->wait_link);
        --global_kernel_state.zombie_task_count;

        context->state = AkCoroutineState::DELETING;
        ct.hdl.destroy();
    }

    
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
AkCoroutineHandle runtime_schedule_next_thread() noexcept {

    // If we have a ready task, resume it
    while (true) {
        if (global_kernel_state.ready_task_count > 0) {
            struct ak_dlink* link = ak_dlink_dequeue(&global_kernel_state.ready_list);
            AkPromise* ctx = runtime_get_linked_task_context(link);
            AkCoroutineHandle task = AkCoroutineHandle::from_promise(*ctx);
            AK_ASSERT(ctx->state == AkCoroutineState::READY);
            ctx->state = AkCoroutineState::RUNNING;
            --global_kernel_state.ready_task_count;
            global_kernel_state.current_task = task;
            //check_invariants();
            return task;
        }

        if (global_kernel_state.iowaiting_task_count > 0) {
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
            
            continue;
        }

        // Zombie bashing
        while (global_kernel_state.zombie_task_count > 0) {
            //dump_task_count();

            struct ak_dlink* zombie_node = ak_dlink_dequeue(&global_kernel_state.zombie_list);
            AkPromise& zombie_promise = *runtime_get_linked_task_context(zombie_node);
            AK_ASSERT(zombie_promise.state == AkCoroutineState::ZOMBIE);

            // Remove from zombie list
            --global_kernel_state.zombie_task_count;
            ak_dlink_detach(&zombie_promise.wait_link);

            // Remove from task list
            ak_dlink_detach(&zombie_promise.tasklist_link);
            --global_kernel_state.task_count;

            // Delete
            zombie_promise.state = AkCoroutineState::DELETING;
            AkCoroutineHandle zombie_task_hdl = AkCoroutineHandle::from_promise(zombie_promise);
            zombie_task_hdl.destroy();

            //dump_task_count();
        }

        if (global_kernel_state.ready_task_count == 0) {
            abort();
        }
    }
    // unreachable
    std::abort();
}
