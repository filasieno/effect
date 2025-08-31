#include "ak/sync/sync.hpp" // IWYU pragma: keep

namespace ak {
    
    // WaitOp
    // ----------------------------------------------------------------------------------------------------------------

    CThread::Hdl op::WaitEvent::await_suspend(CThread::Hdl hdl) const noexcept {
        using namespace priv;

        AkPromise* ctx = &hdl.promise();
        AK_ASSERT(global_kernel_state.current_cthread == hdl);
        AK_ASSERT(ctx->state == AkCoroutineState::RUNNING);
        
        // Move state from RUNNING to WAITING  
        ctx->state = AkCoroutineState::WAITING;
        ++global_kernel_state.waiting_cthread_count;
        ak_enqueue_dlink(&evt->wait_list, &ctx->wait_link);
        global_kernel_state.current_cthread.reset();
        ak::priv::check_invariants();

        return schedule_next_thread();
    }

    // Event routines implementation
    // ----------------------------------------------------------------------------------------------------------------

    AkI32 signal(Event* event) {
        using namespace priv;
        AK_ASSERT(event != nullptr);
        
        if (ak_is_dlink_detached(&event->wait_list)) return 0;

        AkDLink* link = ak_dequeue_dlink(&event->wait_list);
        AkPromise* ctx = ak::priv::get_linked_cthread_context(link);
        AK_ASSERT(ctx->state == AkCoroutineState::WAITING);
        
        // Move the target task from WAITING to READY
        ak_detach_dlink(link);
        --global_kernel_state.waiting_cthread_count;
        ctx->state = AkCoroutineState::READY;
        ak_enqueue_dlink(&global_kernel_state.ready_list, &ctx->wait_link);
        ++global_kernel_state.ready_cthread_count;
        return 1;
    }

    AkI32 signal_n(Event* event, int n) {
        
        using namespace priv;
        AK_ASSERT(event != nullptr);
        AK_ASSERT(n >= 0);
        int count = 0;
        while (count < n && !ak_is_dlink_detached(&event->wait_list)) {
            AkDLink* link = ak_dequeue_dlink(&event->wait_list);
            AkPromise* ctx = ak::priv::get_linked_cthread_context(link);
            AK_ASSERT(ctx->state == AkCoroutineState::WAITING);
            
            // Move the target task from WAITING to READY
            ak_detach_dlink(link);
            --global_kernel_state.waiting_cthread_count;
            ctx->state = AkCoroutineState::READY;
            ak_enqueue_dlink(&global_kernel_state.ready_list, &ctx->wait_link);
            ++global_kernel_state.ready_cthread_count;    
            ++count;
        }
        return count;
    }

    AkI32 signal_all(Event* event) {
        using namespace priv;
        AK_ASSERT(event != nullptr);
        int signalled = 0;
        while (!ak_is_dlink_detached(&event->wait_list)) {
            AkDLink* link = ak_dequeue_dlink(&event->wait_list);
            AkPromise* ctx = ak::priv::get_linked_cthread_context(link);
            AK_ASSERT(ctx->state == AkCoroutineState::WAITING);
            
            // Move the target task from WAITING to READY
            ak_detach_dlink(link);
            --global_kernel_state.waiting_cthread_count;
            ctx->state = AkCoroutineState::READY;
            ak_enqueue_dlink(&global_kernel_state.ready_list, &ctx->wait_link);
            ++global_kernel_state.ready_cthread_count;
            
            ++signalled;        
        }
        return signalled;
    }



}