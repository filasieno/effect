#pragma once

#include "ak/runtime/runtime_api.hpp" // IWYU pragma: keep

namespace ak {
    struct Event {  
        AkDLink wait_list;
    };

    namespace op {
        
        struct WaitEvent {
            explicit WaitEvent(Event* event) : evt(event) {}

            constexpr AkBool  await_ready() const noexcept  { return false; }
            constexpr AkVoid  await_resume() const noexcept { }
            AkCoroutineHandle await_suspend(AkCoroutineHandle hdl) const noexcept;
            

            Event* evt;
        };
    }

    // Concurrency Tools

    AkVoid        init_event(Event* event);
    AkI32         signal(Event* event);
    AkI32         signal_n(Event* event, int n);
    AkI32         signal_all(Event* event);
    op::WaitEvent wait(Event* event);

}


