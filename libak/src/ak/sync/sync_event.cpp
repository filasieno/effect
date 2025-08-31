#include "ak/sync/sync.hpp" // IWYU pragma: keep

namespace ak {
    
    // WaitOp
    // ----------------------------------------------------------------------------------------------------------------

    AkCoroutineHandle op::WaitEvent::await_suspend(AkCoroutineHandle hdl) const noexcept {
        using namespace priv;

        AkPromise* ctx = &hdl.promise();
        AK_ASSERT(global_kernel_state.current_task == hdl);
        AK_ASSERT(ctx->state == AkCoroutineState::RUNNING);
        
        // Move state from RUNNING to WAITING  
        ctx->state = AkCoroutineState::WAITING;
        ++global_kernel_state.waiting_task_count;
        ak_dlink_enqueue(&evt->wait_list, &ctx->wait_link);
        global_kernel_state.current_task.reset();
        runtime_check_invariants();

        return runtime_schedule_next_thread();
    }

    // Event routines implementation
    // ----------------------------------------------------------------------------------------------------------------

    AkI32 signal(Event* event) {
        using namespace priv;
        AK_ASSERT(event != nullptr);
        
        if (ak_dlink_is_detached(&event->wait_list)) return 0;

        AkDLink* link = ak_dlink_dequeue(&event->wait_list);
        AkPromise* ctx = runtime_get_linked_task_context(link);
        AK_ASSERT(ctx->state == AkCoroutineState::WAITING);
        
        // Move the target task from WAITING to READY
        ak_dlink_detach(link);
        --global_kernel_state.waiting_task_count;
        ctx->state = AkCoroutineState::READY;
        ak_dlink_enqueue(&global_kernel_state.ready_list, &ctx->wait_link);
        ++global_kernel_state.ready_task_count;
        return 1;
    }

    AkI32 signal_n(Event* event, int n) {
        
        using namespace priv;
        AK_ASSERT(event != nullptr);
        AK_ASSERT(n >= 0);
        int count = 0;
        while (count < n && !ak_dlink_is_detached(&event->wait_list)) {
            AkDLink* link = ak_dlink_dequeue(&event->wait_list);
            AkPromise* ctx = runtime_get_linked_task_context(link);
            AK_ASSERT(ctx->state == AkCoroutineState::WAITING);
            
            // Move the target task from WAITING to READY
            ak_dlink_detach(link);
            --global_kernel_state.waiting_task_count;
            ctx->state = AkCoroutineState::READY;
            ak_dlink_enqueue(&global_kernel_state.ready_list, &ctx->wait_link);
            ++global_kernel_state.ready_task_count;    
            ++count;
        }
        return count;
    }

    AkI32 signal_all(Event* event) {
        using namespace priv;
        AK_ASSERT(event != nullptr);
        int signalled = 0;
        while (!ak_dlink_is_detached(&event->wait_list)) {
            AkDLink* link = ak_dlink_dequeue(&event->wait_list);
            AkPromise* ctx = runtime_get_linked_task_context(link);
            AK_ASSERT(ctx->state == AkCoroutineState::WAITING);
            
            // Move the target task from WAITING to READY
            ak_dlink_detach(link);
            --global_kernel_state.waiting_task_count;
            ctx->state = AkCoroutineState::READY;
            ak_dlink_enqueue(&global_kernel_state.ready_list, &ctx->wait_link);
            ++global_kernel_state.ready_task_count;
            
            ++signalled;        
        }
        return signalled;
    }



}