#pragma once
#include "ak/api_priv.hpp"

namespace ak {
    
    // WaitOp
    // ----------------------------------------------------------------------------------------------------------------

    inline constexpr TaskHdl WaitEventOp::await_suspend(TaskHdl hdl) const noexcept {
        using namespace priv;
        using namespace utl;

        TaskContext* ctx = &hdl.promise();
        assert(gKernel.currentTaskHdl == hdl);
        assert(ctx->state == TaskState::RUNNING);
        
        // Move state from RUNNING to WAITING  
        ctx->state = TaskState::WAITING;
        ++gKernel.waitingCount;
        EnqueueLink(&evt->waitingList, &ctx->waitLink);
        ClearTaskHdl(&gKernel.currentTaskHdl);
        CheckInvariants();

        return ScheduleNextTask();
    }

    // Event routines implementation
    // ----------------------------------------------------------------------------------------------------------------

    inline void InitEvent(Event* event) {
        using namespace utl;
        InitLink(&event->waitingList);
    }

    inline int SignalEventOne(Event* event) {
        using namespace utl;
        using namespace priv;
        assert(event != nullptr);
        
        if (IsLinkDetached(&event->waitingList)) return 0;

        DLink* link = DequeueLink(&event->waitingList);
        TaskContext* ctx = GetLinkedTaskContext(link);
        assert(ctx->state == TaskState::WAITING);
        
        // Move the target task from WAITING to READY
        DetachLink(link);
        --gKernel.waitingCount;
        ctx->state = TaskState::READY;
        EnqueueLink(&gKernel.readyList, &ctx->waitLink);
        ++gKernel.readyCount;
        return 1;
    }

    inline int SignalEventSome(Event* event, int n) {
        using namespace utl;
        using namespace priv;
        assert(event != nullptr);
        assert(n >= 0);
        int cc = 0;
        while (cc < n && !IsLinkDetached(&event->waitingList)) {
            DLink* link = DequeueLink(&event->waitingList);
            TaskContext* ctx = GetLinkedTaskContext(link);
            assert(ctx->state == TaskState::WAITING);
            
            // Move the target task from WAITING to READY
            DetachLink(link);
            --gKernel.waitingCount;
            ctx->state = TaskState::READY;
            EnqueueLink(&gKernel.readyList, &ctx->waitLink);
            ++gKernel.readyCount;    
            ++cc;
        }
        return cc;
    }

    inline int SignalEventAll(Event* event) {
        using namespace utl;
        using namespace priv;
        assert(event != nullptr);
        int signalled = 0;
        while (!IsLinkDetached(&event->waitingList)) {
            DLink* link = DequeueLink(&event->waitingList);
            TaskContext* ctx = GetLinkedTaskContext(link);
            assert(ctx->state == TaskState::WAITING);
            
            // Move the target task from WAITING to READY
            DetachLink(link);
            --gKernel.waitingCount;
            ctx->state = TaskState::READY;
            EnqueueLink(&gKernel.readyList, &ctx->waitLink);
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