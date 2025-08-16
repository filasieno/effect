#pragma once
#include "ak/api_priv.hpp"

namespace ak {
    
    // WaitOp
    // ----------------------------------------------------------------------------------------------------------------

    inline constexpr TaskHdl WaitEventOp::await_suspend(TaskHdl hdl) const noexcept {
        using namespace priv;

        TaskContext* ctx = &hdl.promise();
        assert(gKernel.currentTaskHdl == hdl);
        assert(ctx->state == TaskState::RUNNING);
        
        // Move state from RUNNING to WAITING  
        ctx->state = TaskState::WAITING;
        ++gKernel.waitingCount;
        utl::enqueue_link(&evt->waitingList, &ctx->waitLink);
        ClearTaskHdl(&gKernel.currentTaskHdl);
        CheckInvariants();

        return ScheduleNextTask();
    }

    // Event routines implementation
    // ----------------------------------------------------------------------------------------------------------------

    inline Void InitEvent(Event* event) {  
        using namespace ak::utl;
        init_link(&event->waitingList);
    }

    inline int SignalEventOne(Event* event) {
        
        using namespace priv;
        assert(event != nullptr);
        
        if (utl::is_link_detached(&event->waitingList)) return 0;

        utl::DLink* link = utl::dequeue_link(&event->waitingList);
        TaskContext* ctx = GetLinkedTaskContext(link);
        assert(ctx->state == TaskState::WAITING);
        
        // Move the target task from WAITING to READY
        utl::detach_link(link);
        --gKernel.waitingCount;
        ctx->state = TaskState::READY;
        utl::enqueue_link(&gKernel.readyList, &ctx->waitLink);
        ++gKernel.readyCount;
        return 1;
    }

    inline int SignalEventSome(Event* event, int n) {
        
        using namespace priv;
        assert(event != nullptr);
        assert(n >= 0);
        int cc = 0;
        while (cc < n && !utl::is_link_detached(&event->waitingList)) {
            utl::DLink* link = utl::dequeue_link(&event->waitingList);
            TaskContext* ctx = GetLinkedTaskContext(link);
            assert(ctx->state == TaskState::WAITING);
            
            // Move the target task from WAITING to READY
            utl::detach_link(link);
            --gKernel.waitingCount;
            ctx->state = TaskState::READY;
            utl::enqueue_link(&gKernel.readyList, &ctx->waitLink);
            ++gKernel.readyCount;    
            ++cc;
        }
        return cc;
    }

    inline int SignalEventAll(Event* event) {
        using namespace priv;
        assert(event != nullptr);
        int signalled = 0;
        while (!utl::is_link_detached(&event->waitingList)) {
            utl::DLink* link = utl::dequeue_link(&event->waitingList);
            TaskContext* ctx = GetLinkedTaskContext(link);
            assert(ctx->state == TaskState::WAITING);
            
            // Move the target task from WAITING to READY
            utl::detach_link(link);
            --gKernel.waitingCount;
            ctx->state = TaskState::READY;
            utl::enqueue_link(&gKernel.readyList, &ctx->waitLink);
            ++gKernel.readyCount;
            
            ++signalled;        
        }
        return signalled;
    }

    inline WaitEventOp WaitEvent(Event* event) {
        assert(event != nullptr);
        return WaitEventOp{event};
    }

}