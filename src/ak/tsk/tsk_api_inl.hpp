#pragma once

// Public inline API implementation
// --------------------------------

#include "ak/base/base_api.hpp"

namespace ak { 

    template <typename... Args>
    CThread::Context::Context(Args&&... ) {
        using namespace priv;

        init_dlink(&tasklist_link);
        init_dlink(&wait_link);  
        init_dlink(&awaiter_list);
        state = CThread::State::CREATED;
        prepared_io = 0;
        res = -1;

        // Check post-conditions
        AK_ASSERT(is_dlink_detached(&tasklist_link));
        AK_ASSERT(is_dlink_detached(&wait_link));
        AK_ASSERT(state == CThread::State::CREATED);
        // check_invariants();
    }

    inline const Char* to_string(CThread::State state) noexcept 
    {
        switch (state) {
            case CThread::State::INVALID:    return "INVALID";
            case CThread::State::CREATED:    return "CREATED";
            case CThread::State::READY:      return "READY";
            case CThread::State::RUNNING:    return "RUNNING";
            case CThread::State::IO_WAITING: return "IO_WAITING";
            case CThread::State::WAITING:    return "WAITING";
            case CThread::State::ZOMBIE:     return "ZOMBIE";
            case CThread::State::DELETING:   return "DELETING";
            default: return nullptr;
        }
    }

    inline Bool is_valid(CThread ct) noexcept { return ct.hdl.address() != nullptr; }

    inline CThread::Context* get_context(CThread ct) noexcept { return &ct.hdl.promise(); }

    inline CThread::Context* get_context() noexcept { return &global_kernel_state.current_cthread.hdl.promise(); }

    inline constexpr op::GetCurrentTask get_cthread_context_async() noexcept { return {}; }

    inline constexpr op::Suspend suspend() noexcept { return {}; }

    inline op::JoinCThread join(CThread ct) noexcept { return op::JoinCThread(ct); }

    inline op::JoinCThread operator co_await(CThread ct) noexcept { return op::JoinCThread(ct); }

    inline CThread::State get_state(CThread ct) noexcept { return ct.hdl.promise().state; }

    inline Bool is_done(CThread ct) noexcept { return ct.hdl.done(); }

    inline op::ResumeCThread resume(CThread ct) noexcept { return op::ResumeCThread(ct); }

}
