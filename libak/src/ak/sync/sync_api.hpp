#pragma once

#include "ak/runtime/runtime_api.hpp" // IWYU pragma: keep

struct AkEvent {  
    struct ak_dlink wait_list;
};

struct AkWaitEventOp {
    explicit AkWaitEventOp(AkEvent* event) : evt(event) {}

    constexpr bool  await_ready() const noexcept  { return false; }
    constexpr void  await_resume() const noexcept { }
    AkCoroutineHandle await_suspend(AkCoroutineHandle hdl) const noexcept;
    

    AkEvent* evt;
};
void        ak_init_event(AkEvent* event);
int         ak_signal_event(AkEvent* event);
int         ak_signal_event_n(AkEvent* event, int n);
int         ak_signal_event_all(AkEvent* event);
AkWaitEventOp ak_wait_event(AkEvent* event);
