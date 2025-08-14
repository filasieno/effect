#pragma once

#include "dlist.hpp"
#include "types.hpp"

namespace ak {

    struct Event {  
        utl::DLink waitingList;
    };

    struct WaitOp {
        WaitOp(Event* event) : evt(event) {}

        constexpr bool    await_ready() const noexcept;
        constexpr TaskHdl await_suspend(TaskHdl hdl) const noexcept;
        constexpr void    await_resume() const noexcept;

        Event* evt;
    };

    void   InitEvent(Event* event);
    int    SignalOne(Event* event);
    int    SignalSome(Event* event, int n);
    int    SignalAll(Event* event);
    WaitOp WaitEvent(Event* event);
}


// ------------------------------------------------------------------------------------------------
// Implementation
// ------------------------------------------------------------------------------------------------
namespace ak {
    inline constexpr bool WaitOp::await_ready() const noexcept { 
        return false; 
    }

    inline constexpr TaskHdl WaitOp::await_suspend(TaskHdl hdl) const noexcept {
        using namespace internal;
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

    inline constexpr void WaitOp::await_resume() const noexcept { }

    inline void InitEvent(Event* event) {
        using namespace ak::utl;
        InitLink(&event->waitingList);
    }

    inline int SignalOne(Event* event) {
        using namespace ak::utl;
        using namespace ak::internal;
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

    inline int SignalSome(Event* event, int n) {
        using namespace ak::utl;
        using namespace ak::internal;
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

    inline int SignalAll(Event* event) {
        using namespace ak::utl;
        using namespace ak::internal;
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

    inline WaitOp WaitEvent(Event* event) {
        using namespace ak::utl;
        assert(event != nullptr);
        return WaitOp{event};
    }

}