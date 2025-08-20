#pragma once

#include <cassert>
#include "ak/defs.hpp" // IWYU pragma: keep

namespace ak { namespace utl {

    struct DLink {
        DLink* next;
        DLink* prev;
    };

    Void   init_dlink(DLink* link) noexcept;
    Bool   is_dlink_detached(const DLink* link) noexcept;
    Void   detach_dlink(DLink* link) noexcept;
    Void   clear_dlink(DLink* link) noexcept;
    Void   enqueue_dlink(DLink* queue, DLink* link) noexcept;
    DLink* dequeue_dlink(DLink* queue) noexcept;
    Void   insert_prev_dlink(DLink* list, DLink* link) noexcept;
    Void   insert_next_dlink(DLink* list, DLink* link) noexcept;
    Void   push_dlink(DLink* stack, DLink* link) noexcept;
    DLink* pop_dlink(DLink* stack) noexcept;

}} // namespace ak::utl

