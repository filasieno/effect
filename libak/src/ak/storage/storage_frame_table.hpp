#pragma once

#include <ak/storage/storage_api.hpp>
#include <ak/alloc/alloc.hpp>


namespace ak {

    enum class AkBufferPool {
        INVALID = 0,
        DEFAULT,
        RECYCLE,
        KEEP
    };
    const char* ak_to_string(AkBufferPool p) noexcept;

    struct FrameEntry {
        AkU32   pool      : 2;
        AkU32   is_dirty  : 1;
        AkU32   evict     : 1;
        AkU32   pin_count : 28;
        FrameId pool_index;
        PageId  page_cache_bucket;
        VPageId vpage_cache_bucket;
    };
    static_assert(sizeof(FrameEntry) == 16, "FrameEntry must have a size of 16");

    struct FramePool {
        FrameId* entries;
        AkU32    count;
        AkU32    capacity;
    };

    struct FrameTable {
        FrameEntry* entries;
        FramePool   free_pool;
        FramePool   default_pool;
        FramePool   recycle_pool;
        FramePool   keep_pool;
        AkU32       clock;
    };

    // Frametable
    void        frametable_init(FrameTable* ft, AkU32 capacity, AkAllocTable* at) noexcept;
    void        frametable_fini(FrameTable* ft, AkAllocTable* at) noexcept;
    void        frametable_dump_debug(const FrameTable* ft) noexcept;
    FrameId     frametable_allocate_frame(FrameTable* ft, AkBufferPool pool) noexcept;
    void        frametable_freeframe(FrameTable* ft, FrameId frame_id) noexcept;
    FrameEntry* frametable_get_entry(FrameTable* ft, FrameId frame_id) noexcept;
    AkU32       frametable_get_capacity(const FrameTable* ft) noexcept;
    AkU32       frametable_get_free_count(const FrameTable* ft) noexcept;
    void        frametable_validate_frame_id(const FrameTable* ft, FrameId frame_id) noexcept;
    void        frametable_move_to_pool(FrameTable* ft, FrameId frame_id, AkBufferPool dest_pool_type) noexcept;

    // FramePool
    void        framepool_init(FramePool* framePool, AkU32 capacity, AkAllocTable* at) noexcept;
    void        framepool_fini(FramePool* pool, AkAllocTable* at) noexcept;
    AkBool      framepool_is_full(const FramePool* pool) noexcept;

    void        framepool_free_check_invariants(const FrameTable* ft) noexcept;
    void        framepool_keep_check_invariants(const FrameTable* ft) noexcept;
    void        framepool_recycle_check_invariants(const FrameTable* ft) noexcept;
    void        framepool_check_invariants(const FrameTable* ft) noexcept;

} // namespace ak
