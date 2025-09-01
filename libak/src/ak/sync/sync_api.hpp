#pragma once

#include "ak/runtime/runtime_api.hpp" // IWYU pragma: keep

struct AkEvent {  
    AkDLink wait_list;
};

struct AkWaitEventOp {
    explicit AkWaitEventOp(AkEvent* event) : evt(event) {}

    constexpr AkBool  await_ready() const noexcept  { return false; }
    constexpr AkVoid  await_resume() const noexcept { }
    AkCoroutineHandle await_suspend(AkCoroutineHandle hdl) const noexcept;
    

    AkEvent* evt;
};
AkVoid        ak_init_event(AkEvent* event);
AkI32         ak_signal_event(AkEvent* event);
AkI32         ak_signal_event_n(AkEvent* event, int n);
AkI32         ak_signal_event_all(AkEvent* event);
AkWaitEventOp ak_wait_event(AkEvent* event);
