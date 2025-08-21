#pragma once

#include "ak/runtime/runtime_api.hpp" // IWYU pragma: keep

namespace ak {
    struct Event {  
        priv::DLink wait_list;
    };

    namespace op {
        
        struct WaitEvent {
            explicit WaitEvent(Event* event) : evt(event) {}

            constexpr Bool await_ready() const noexcept  { return false; }
            constexpr Void await_resume() const noexcept { }
            CThread::Hdl   await_suspend(CThread::Hdl hdl) const noexcept;
            

            Event* evt;
        };
    }

    // Concurrency Tools

    Void          init_event(Event* event);
    I32           signal(Event* event);
    I32           signal_n(Event* event, int n);
    I32           signal_all(Event* event);
    op::WaitEvent wait(Event* event);

}


