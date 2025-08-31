#pragma once

#include <ak/storage/storage_api.hpp>
#include <ak/alloc/alloc.hpp>


namespace ak {

    enum class BufferPool {
        INVALID = 0,
        DEFAULT,
        RECYCLE,
        KEEP
    };
    const char* to_string(BufferPool p) noexcept;

    struct FrameEntry {
        AkU32        pool      : 2;
        AkU32        is_dirty  : 1;
        AkU32        evict     : 1;
        AkU32        pin_count : 28;
        FrameId    pool_index;
        PageId     page_cache_bucket;
        VPageId    vpage_cache_bucket;
    };
    static_assert(sizeof(FrameEntry) == 16, "FrameEntry must have a size of 16");

    struct FramePool {
        FrameId* entries;
        AkU32      count;
        AkU32      capacity;
    };

    struct FrameTable {
        FrameEntry* entries;
        FramePool   free_pool;
        FramePool   default_pool;
        FramePool   recycle_pool;
        FramePool   keep_pool;
        AkU32         clock;
    };

    void        init_frame_table(FrameTable* ft, AkU32 capacity, AllocTable* at) noexcept;
    void        fini_frame_table(FrameTable* ft, AllocTable* at) noexcept;
    void        dump_frame_table_debug(const FrameTable* ft) noexcept;
    FrameId     allocate_frame(FrameTable* ft, BufferPool pool) noexcept;
    void        free_frame(FrameTable* ft, FrameId frame_id) noexcept;
    FrameEntry* get_frame_entry(FrameTable* ft, FrameId frame_id) noexcept;
    AkU32         get_frame_table_capacity(const FrameTable* ft) noexcept;
    AkU32         get_frame_table_free_count(const FrameTable* ft) noexcept;
    void        validate_frame_id(const FrameTable* ft, FrameId frame_id) noexcept;
    void        move_frame_to_pool(FrameTable* ft, FrameId frame_id, BufferPool dest_pool_type) noexcept;

    // FramePool
    void        init_frame_pool(FramePool* framePool, AkU32 capacity, AllocTable* at) noexcept;
    void        fini_frame_pool(FramePool* pool, AllocTable* at) noexcept;
    AkBool        is_frame_pool_full(const FramePool* pool) noexcept;

    void        check_invariants_free_pool(const FrameTable* ft) noexcept;
    void        check_invariants_keep_pool(const FrameTable* ft) noexcept;
    void        check_invariants_recycle_pool(const FrameTable* ft) noexcept;
    void        check_invariants_pool_capacity(const FrameTable* ft) noexcept;

} // namespace ak
