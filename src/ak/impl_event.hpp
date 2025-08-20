#pragma once
#include "ak/api_priv.hpp"

namespace ak {
    
    // WaitOp
    // ----------------------------------------------------------------------------------------------------------------

    inline constexpr CThread::Hdl op::WaitEvent::await_suspend(CThread::Hdl hdl) const noexcept {
        using namespace priv;

        CThread::Context* ctx = &hdl.promise();
        assert(global_kernel_state.current_cthread == hdl);
        assert(ctx->state == CThread::State::RUNNING);
        
        // Move state from RUNNING to WAITING  
        ctx->state = CThread::State::WAITING;
        ++global_kernel_state.waiting_cthread_count;
        utl::enqueue_dlink(&evt->wait_list, &ctx->wait_link);
        global_kernel_state.current_cthread.reset();
        check_invariants();

        return schedule_cthread();
    }

    // Event routines implementation
    // ----------------------------------------------------------------------------------------------------------------

    inline Void init(Event* event) {  
        using namespace ak::utl;
        init_dlink(&event->wait_list);
    }

    inline int signal(Event* event) {
        
        using namespace priv;
        assert(event != nullptr);
        
        if (utl::is_dlink_detached(&event->wait_list)) return 0;

        utl::DLink* link = utl::dequeue_dlink(&event->wait_list);
        CThread::Context* ctx = get_linked_cthread_context(link);
        assert(ctx->state == CThread::State::WAITING);
        
        // Move the target task from WAITING to READY
        utl::detach_dlink(link);
        --global_kernel_state.waiting_cthread_count;
        ctx->state = CThread::State::READY;
        utl::enqueue_dlink(&global_kernel_state.ready_list, &ctx->wait_link);
        ++global_kernel_state.ready_cthread_count;
        return 1;
    }

    inline int signal_n(Event* event, int n) {
        
        using namespace priv;
        assert(event != nullptr);
        assert(n >= 0);
        int count = 0;
        while (count < n && !utl::is_dlink_detached(&event->wait_list)) {
            utl::DLink* link = utl::dequeue_dlink(&event->wait_list);
            CThread::Context* ctx = get_linked_cthread_context(link);
            assert(ctx->state == CThread::State::WAITING);
            
            // Move the target task from WAITING to READY
            utl::detach_dlink(link);
            --global_kernel_state.waiting_cthread_count;
            ctx->state = CThread::State::READY;
            utl::enqueue_dlink(&global_kernel_state.ready_list, &ctx->wait_link);
            ++global_kernel_state.ready_cthread_count;    
            ++count;
        }
        return count;
    }

    inline int signal_all(Event* event) {
        using namespace priv;
        assert(event != nullptr);
        int signalled = 0;
        while (!utl::is_dlink_detached(&event->wait_list)) {
            utl::DLink* link = utl::dequeue_dlink(&event->wait_list);
            CThread::Context* ctx = get_linked_cthread_context(link);
            assert(ctx->state == CThread::State::WAITING);
            
            // Move the target task from WAITING to READY
            utl::detach_dlink(link);
            --global_kernel_state.waiting_cthread_count;
            ctx->state = CThread::State::READY;
            utl::enqueue_dlink(&global_kernel_state.ready_list, &ctx->wait_link);
            ++global_kernel_state.ready_cthread_count;
            
            ++signalled;        
        }
        return signalled;
    }

    inline op::WaitEvent wait(Event* event) {
        assert(event != nullptr);
        return op::WaitEvent{event};
    }

}