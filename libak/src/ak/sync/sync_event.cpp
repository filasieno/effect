#include "ak/sync/sync.hpp" // IWYU pragma: keep

namespace ak {
    
    // WaitOp
    // ----------------------------------------------------------------------------------------------------------------

    CThread::Hdl op::WaitEvent::await_suspend(CThread::Hdl hdl) const noexcept {
        using namespace priv;

        CThread::Context* ctx = &hdl.promise();
        AK_ASSERT(global_kernel_state.current_cthread == hdl);
        AK_ASSERT(ctx->state == CThread::State::RUNNING);
        
        // Move state from RUNNING to WAITING  
        ctx->state = CThread::State::WAITING;
        ++global_kernel_state.waiting_cthread_count;
        enqueue_AkDLink(&evt->wait_list, &ctx->wait_link);
        global_kernel_state.current_cthread.reset();
        check_invariants();

        return schedule_next_thread();
    }

    // Event routines implementation
    // ----------------------------------------------------------------------------------------------------------------

    AkI32 signal(Event* event) {
        using namespace priv;
        AK_ASSERT(event != nullptr);
        
        if (is_AkDLink_detached(&event->wait_list)) return 0;

        priv::AkDLink* link = dequeue_AkDLink(&event->wait_list);
        CThread::Context* ctx = get_linked_cthread_context(link);
        AK_ASSERT(ctx->state == CThread::State::WAITING);
        
        // Move the target task from WAITING to READY
        detach_AkDLink(link);
        --global_kernel_state.waiting_cthread_count;
        ctx->state = CThread::State::READY;
        enqueue_AkDLink(&global_kernel_state.ready_list, &ctx->wait_link);
        ++global_kernel_state.ready_cthread_count;
        return 1;
    }

    AkI32 signal_n(Event* event, int n) {
        
        using namespace priv;
        AK_ASSERT(event != nullptr);
        AK_ASSERT(n >= 0);
        int count = 0;
        while (count < n && !is_AkDLink_detached(&event->wait_list)) {
            priv::AkDLink* link = dequeue_AkDLink(&event->wait_list);
            CThread::Context* ctx = get_linked_cthread_context(link);
            AK_ASSERT(ctx->state == CThread::State::WAITING);
            
            // Move the target task from WAITING to READY
            detach_AkDLink(link);
            --global_kernel_state.waiting_cthread_count;
            ctx->state = CThread::State::READY;
            enqueue_AkDLink(&global_kernel_state.ready_list, &ctx->wait_link);
            ++global_kernel_state.ready_cthread_count;    
            ++count;
        }
        return count;
    }

    AkI32 signal_all(Event* event) {
        using namespace priv;
        AK_ASSERT(event != nullptr);
        int signalled = 0;
        while (!is_AkDLink_detached(&event->wait_list)) {
            priv::AkDLink* link = dequeue_AkDLink(&event->wait_list);
            CThread::Context* ctx = get_linked_cthread_context(link);
            AK_ASSERT(ctx->state == CThread::State::WAITING);
            
            // Move the target task from WAITING to READY
            detach_AkDLink(link);
            --global_kernel_state.waiting_cthread_count;
            ctx->state = CThread::State::READY;
            enqueue_AkDLink(&global_kernel_state.ready_list, &ctx->wait_link);
            ++global_kernel_state.ready_cthread_count;
            
            ++signalled;        
        }
        return signalled;
    }



}