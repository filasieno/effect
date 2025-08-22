#include "ak/runtime/runtime.hpp" // IWYU pragma: keep

#include <print>

namespace ak { 
   
    Void* BootCThread::Context::operator new(std::size_t n) noexcept {
        AK_ASSERT(n <= sizeof(global_kernel_state.boot_cthread_frame_buffer));
        return (Void*)global_kernel_state.boot_cthread_frame_buffer;
    }    

}

// Kernel boot implementation 
// ================================================================================================================

namespace ak::priv {

    // RunSchedulerTaskOp
    // ----------------------------------------------------------------------------------------------------------------

    CThread::Hdl RunSchedulerOp::await_suspend(BootCThread::Hdl current_task_hdl) const noexcept {
        using namespace priv;

        (Void)current_task_hdl;
        CThread::Context* scheduler_ctx = get_context(global_kernel_state.scheduler_cthread);

        // Check expected state post scheduler construction

        AK_ASSERT(global_kernel_state.cthread_count == 1);
        AK_ASSERT(global_kernel_state.ready_cthread_count == 1);
        AK_ASSERT(scheduler_ctx->state == CThread::State::READY);
        AK_ASSERT(!is_dlink_detached(&scheduler_ctx->wait_link));
        AK_ASSERT(global_kernel_state.current_cthread == CThread::Hdl());

        // Setup SchedulerTask for execution (from READY -> RUNNING)
        global_kernel_state.current_cthread = global_kernel_state.scheduler_cthread;
        scheduler_ctx->state = CThread::State::RUNNING;
        detach_dlink(&scheduler_ctx->wait_link);
        --global_kernel_state.ready_cthread_count;

        // Check expected state post task system bootstrap
        //check_invariants();
        return global_kernel_state.scheduler_cthread;
    }

    // TerminateSchedulerOp
    // ----------------------------------------------------------------------------------------------------------------

    BootCThread::Hdl TerminateSchedulerOp::await_suspend(CThread::Hdl hdl) const noexcept {
        using namespace priv;

        AK_ASSERT(global_kernel_state.current_cthread == global_kernel_state.scheduler_cthread);
        AK_ASSERT(global_kernel_state.current_cthread == hdl);

        auto* scheduler_context = get_context(global_kernel_state.scheduler_cthread);
        AK_ASSERT(scheduler_context->state == CThread::State::RUNNING);
        AK_ASSERT(is_dlink_detached(&scheduler_context->wait_link));

        scheduler_context->state = CThread::State::ZOMBIE;
        global_kernel_state.current_cthread.reset();
        enqueue_dlink(&global_kernel_state.zombie_list, &scheduler_context->wait_link);
        ++global_kernel_state.zombie_cthread_count;

        return global_kernel_state.boot_cthread;
    }

    // Boot implementation
    // ----------------------------------------------------------------------------------------------------------------

    Void destroy_scheduler(CThread ct) noexcept {
        using namespace priv;
        auto* context = get_context(ct);

        // Remove from Task list
        detach_dlink(&context->tasklist_link);
        --global_kernel_state.cthread_count;

        // Remove from Zombie List
        detach_dlink(&context->wait_link);
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
    CThread::Hdl schedule_next_thread() noexcept {
        using namespace priv;

        // If we have a ready task, resume it
        while (true) {
            if (global_kernel_state.ready_cthread_count > 0) {
                priv::DLink* link = dequeue_dlink(&global_kernel_state.ready_list);
                CThread::Context* ctx = get_linked_cthread_context(link);
                CThread::Hdl task = CThread::Hdl::from_promise(*ctx);
                AK_ASSERT(ctx->state == CThread::State::READY);
                ctx->state = CThread::State::RUNNING;
                --global_kernel_state.ready_cthread_count;
                global_kernel_state.current_cthread = task;
                //check_invariants();
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
                
                continue;
            }

            // Zombie bashing
            while (global_kernel_state.zombie_cthread_count > 0) {
                //dump_task_count();

                priv::DLink* zombie_node = dequeue_dlink(&global_kernel_state.zombie_list);
                CThread::Context& zombie_promise = *get_linked_cthread_context(zombie_node);
                AK_ASSERT(zombie_promise.state == CThread::State::ZOMBIE);

                // Remove from zombie list
                --global_kernel_state.zombie_cthread_count;
                detach_dlink(&zombie_promise.wait_link);

                // Remove from task list
                detach_dlink(&zombie_promise.tasklist_link);
                --global_kernel_state.cthread_count;

                // Delete
                zombie_promise.state = CThread::State::DELETING;
                CThread::Hdl zombie_task_hdl = CThread::Hdl::from_promise(zombie_promise);
                zombie_task_hdl.destroy();

                //dump_task_count();
            }

            if (global_kernel_state.ready_cthread_count == 0) {
                abort();
            }
        }
        // unreachable
        abort();
    }
}
