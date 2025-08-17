#pragma once

#include <cassert>
#include "ak/defs.hpp" // IWYU pragma: keep

namespace ak { namespace utl {

    struct DLink {
        DLink* next;
        DLink* prev;
    };

    Void   init_link(DLink* link) noexcept;
    Bool   is_link_detached(const DLink* link) noexcept;
    Void   detach_link(DLink* link) noexcept;
    Void   clear_link(DLink* link) noexcept;
    Void   enqueue_link(DLink* queue, DLink* link) noexcept;
    DLink* dequeue_link(DLink* queue) noexcept;
    Void   insert_prev_link(DLink* queue, DLink* link) noexcept;
    Void   insert_next_link(DLink* queue, DLink* link) noexcept;
    Void   push_link(DLink* stack, DLink* link) noexcept;
    DLink* pop_link(DLink* stack) noexcept;

}} // namespace ak::utl

