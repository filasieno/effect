#pragma once

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
        enqueue_dlink(&evt->wait_list, &ctx->wait_link);
        global_kernel_state.current_cthread.reset();
        check_invariants();

        return schedule_cthread();
    }

    // Event routines implementation
    // ----------------------------------------------------------------------------------------------------------------

    I32 signal(Event* event) {
        using namespace priv;
        AK_ASSERT(event != nullptr);
        
        if (is_dlink_detached(&event->wait_list)) return 0;

        priv::DLink* link = dequeue_dlink(&event->wait_list);
        CThread::Context* ctx = get_linked_cthread_context(link);
        AK_ASSERT(ctx->state == CThread::State::WAITING);
        
        // Move the target task from WAITING to READY
        detach_dlink(link);
        --global_kernel_state.waiting_cthread_count;
        ctx->state = CThread::State::READY;
        enqueue_dlink(&global_kernel_state.ready_list, &ctx->wait_link);
        ++global_kernel_state.ready_cthread_count;
        return 1;
    }

    I32 signal_n(Event* event, int n) {
        
        using namespace priv;
        AK_ASSERT(event != nullptr);
        AK_ASSERT(n >= 0);
        int count = 0;
        while (count < n && !is_dlink_detached(&event->wait_list)) {
            priv::DLink* link = dequeue_dlink(&event->wait_list);
            CThread::Context* ctx = get_linked_cthread_context(link);
            AK_ASSERT(ctx->state == CThread::State::WAITING);
            
            // Move the target task from WAITING to READY
            detach_dlink(link);
            --global_kernel_state.waiting_cthread_count;
            ctx->state = CThread::State::READY;
            enqueue_dlink(&global_kernel_state.ready_list, &ctx->wait_link);
            ++global_kernel_state.ready_cthread_count;    
            ++count;
        }
        return count;
    }

    I32 signal_all(Event* event) {
        using namespace priv;
        AK_ASSERT(event != nullptr);
        int signalled = 0;
        while (!is_dlink_detached(&event->wait_list)) {
            priv::DLink* link = dequeue_dlink(&event->wait_list);
            CThread::Context* ctx = get_linked_cthread_context(link);
            AK_ASSERT(ctx->state == CThread::State::WAITING);
            
            // Move the target task from WAITING to READY
            detach_dlink(link);
            --global_kernel_state.waiting_cthread_count;
            ctx->state = CThread::State::READY;
            enqueue_dlink(&global_kernel_state.ready_list, &ctx->wait_link);
            ++global_kernel_state.ready_cthread_count;
            
            ++signalled;        
        }
        return signalled;
    }



}