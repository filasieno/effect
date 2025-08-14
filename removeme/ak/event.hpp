#pragma once

#include "ak/core.hpp"

namespace ak {

struct Event {  
    internal::DLink waitingList;
};

inline void InitEvent(Event* event) {
    internal::InitLink(&event->waitingList);
}

inline int SignalOne(Event* event) {
    using namespace internal;
    assert(event != nullptr);
    if (IsLinkDetached(&event->waitingList)) return 0;
    DLink* link = DequeueLink(&event->waitingList);
    TaskContext* ctx = GetLinkedTaskContext(link);
    assert(ctx->state == TaskState::WAITING);
    DetachLink(link);
    --gKernel.waitingCount;
    ctx->state = TaskState::READY;
    EnqueueLink(&gKernel.readyList, &ctx->waitLink);
    ++gKernel.readyCount;
    return 1;
}

inline int SignalSome(Event* event, int n) {
    using namespace internal;
    assert(event != nullptr);
    assert(n >= 0);
    int cc = 0;
    while (cc < n && !IsLinkDetached(&event->waitingList)) {
        DLink* link = DequeueLink(&event->waitingList);
        TaskContext* ctx = GetLinkedTaskContext(link);
        assert(ctx->state == TaskState::WAITING);
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
    using namespace internal;
    assert(event != nullptr);
    int signalled = 0;
    while (!IsLinkDetached(&event->waitingList)) {
        DLink* link = DequeueLink(&event->waitingList);
        TaskContext* ctx = GetLinkedTaskContext(link);
        assert(ctx->state == TaskState::WAITING);
        DetachLink(link);
        --gKernel.waitingCount;
        ctx->state = TaskState::READY;
        EnqueueLink(&gKernel.readyList, &ctx->waitLink);
        ++gKernel.readyCount;
        ++signalled;        
    }
    return signalled;
}

inline auto WaitEvent(Event* event) {
    using namespace internal;
    assert(event != nullptr);
    struct WaitOp {
        WaitOp(Event* event) : evt(event) {}
        constexpr bool await_ready() const noexcept { return false; }
        constexpr TaskHdl await_suspend(TaskHdl hdl) const noexcept {
            using namespace internal;
            TaskContext* ctx = &hdl.promise();
            assert(gKernel.currentTaskHdl == hdl);
            assert(ctx->state == TaskState::RUNNING);
            ctx->state = TaskState::WAITING;
            ++gKernel.waitingCount;
            EnqueueLink(&evt->waitingList, &ctx->waitLink);
            ClearTaskHdl(&gKernel.currentTaskHdl);
            CheckInvariants();
            return ScheduleNextTask();
        }
        constexpr void await_resume() const noexcept { }
        Event* evt;
    };
    return WaitOp{event};
}

} // namespace ak



