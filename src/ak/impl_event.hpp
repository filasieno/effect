#pragma once
#include "ak/api_priv.hpp"

namespace ak {
    
    // WaitOp
    // ----------------------------------------------------------------------------------------------------------------

    inline constexpr CThreadCtxHdl WaitEventOp::await_suspend(CThreadCtxHdl hdl) const noexcept {
        using namespace priv;

        CThreadContext* ctx = &hdl.promise();
        assert(gKernel.current_ctx_hdl == hdl);
        assert(ctx->state == TaskState::RUNNING);
        
        // Move state from RUNNING to WAITING  
        ctx->state = TaskState::WAITING;
        ++gKernel.waiting_count;
        utl::enqueue_link(&evt->wait_list, &ctx->wait_link);
        clear(&gKernel.current_ctx_hdl);
        check_invariants();

        return schedule();
    }

    // Event routines implementation
    // ----------------------------------------------------------------------------------------------------------------

    inline Void init(Event* event) {  
        using namespace ak::utl;
        init_link(&event->wait_list);
    }

    inline int signal(Event* event) {
        
        using namespace priv;
        assert(event != nullptr);
        
        if (utl::is_link_detached(&event->wait_list)) return 0;

        utl::DLink* link = utl::dequeue_link(&event->wait_list);
        CThreadContext* ctx = get_linked_context(link);
        assert(ctx->state == TaskState::WAITING);
        
        // Move the target task from WAITING to READY
        utl::detach_link(link);
        --gKernel.waiting_count;
        ctx->state = TaskState::READY;
        utl::enqueue_link(&gKernel.ready_list, &ctx->wait_link);
        ++gKernel.ready_count;
        return 1;
    }

    inline int signal_n(Event* event, int n) {
        
        using namespace priv;
        assert(event != nullptr);
        assert(n >= 0);
        int cc = 0;
        while (cc < n && !utl::is_link_detached(&event->wait_list)) {
            utl::DLink* link = utl::dequeue_link(&event->wait_list);
            CThreadContext* ctx = get_linked_context(link);
            assert(ctx->state == TaskState::WAITING);
            
            // Move the target task from WAITING to READY
            utl::detach_link(link);
            --gKernel.waiting_count;
            ctx->state = TaskState::READY;
            utl::enqueue_link(&gKernel.ready_list, &ctx->wait_link);
            ++gKernel.ready_count;    
            ++cc;
        }
        return cc;
    }

    inline int signal_all(Event* event) {
        using namespace priv;
        assert(event != nullptr);
        int signalled = 0;
        while (!utl::is_link_detached(&event->wait_list)) {
            utl::DLink* link = utl::dequeue_link(&event->wait_list);
            CThreadContext* ctx = get_linked_context(link);
            assert(ctx->state == TaskState::WAITING);
            
            // Move the target task from WAITING to READY
            utl::detach_link(link);
            --gKernel.waiting_count;
            ctx->state = TaskState::READY;
            utl::enqueue_link(&gKernel.ready_list, &ctx->wait_link);
            ++gKernel.ready_count;
            
            ++signalled;        
        }
        return signalled;
    }

    inline WaitEventOp wait(Event* event) {
        assert(event != nullptr);
        return WaitEventOp{event};
    }

}