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

/*
    template <typename Key, typename Value>
    struct hash_map {
        DLink* buckets;
        U32   bucket_count;
        U32   bucket_mask;
        U32   bucket_shift;
        U32   bucket_size;
    };

    Void init(hash_map* map, U32 bucket_count) noexcept;
    Void clear_hash_map(hash_map* map) noexcept;
    Void put(hash_map* map, U64 key, U64 value) noexcept;
    U64* lookup(hash_map* map, U64 key) noexcept;
    Void remove(hash_map* map, U64 key) noexcept;
*/

}} // namespace ak::utl

