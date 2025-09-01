#include "ak/storage/storage_api_priv.hpp" // IWYU pragma: keep
#include <bit>
#include <print>
#include <algorithm> // for std::fill

inline static AkFramePool* get_pool(AkFrameTable *ft, AkBufferPool pool) noexcept {
    switch (pool) {
        case AkBufferPool::INVALID: return &ft->free_pool;
        case AkBufferPool::DEFAULT: return &ft->default_pool;
        case AkBufferPool::RECYCLE: return &ft->recycle_pool;
        case AkBufferPool::KEEP: return &ft->keep_pool;
    }
    std::abort();
    // Unreachable
    return nullptr;
}

inline static AkFrameEntry* get_entry(AkFrameTable *ft, AkFrameId frame_id) noexcept {
    storage_frametable_validate_frame_id(ft, frame_id);
    return &ft->entries[frame_id.id];
}

AkVoid storage_framepool_init(AkFramePool *fp, AkU32 capacity, AkAllocTable *at) noexcept {
    AkU32 aligned_capacity = std::bit_ceil(static_cast<AkU32>(capacity));
    AkSize byte_size = static_cast<AkSize>(aligned_capacity) * sizeof(AkFrameId);
    AkFrameId *e = static_cast<AkFrameId *>(alloc_table_try_malloc(at, byte_size));
    AK_ASSERT(e != nullptr);
    std::fill(e, e + aligned_capacity, AkFrameId());
    fp->entries = e;
    fp->count = 0;
    fp->capacity = aligned_capacity;
}

AkVoid storage_framepool_fini(AkFramePool *fp, AkAllocTable *at) noexcept {
    alloc_table_free(at, fp->entries, 0);
    fp->entries = nullptr;
    fp->count = 0;
    fp->capacity = 0;
}

AkBool storage_framepool_is_full(const AkFramePool *fp) noexcept { return fp->count == fp->capacity; }

AkVoid frametable_init(AkFrameTable *ft, AkU32 capacity, AkAllocTable *at) noexcept {

    AkU32 aligned = std::bit_ceil(static_cast<AkU32>(capacity));
    AkSize byte_size = static_cast<AkSize>(aligned) * sizeof(AkFrameEntry);
    AkFrameEntry *entries = static_cast<AkFrameEntry *>(alloc_table_try_malloc(at, byte_size));
    AK_ASSERT(entries != nullptr);

    storage_framepool_init(&ft->free_pool, aligned, at);
    storage_framepool_init(&ft->default_pool, aligned, at);
    storage_framepool_init(&ft->recycle_pool, aligned, at);
    storage_framepool_init(&ft->keep_pool, aligned, at);

    AkU32 entry_id = 0;
    AkU32 free_pool_slot = aligned - 1;
    while (true) {
        AkFrameEntry *entry = &entries[entry_id];
        *entry = AkFrameEntry();
        entry->pool_index = AkFrameId(free_pool_slot);
        entry->pool = static_cast<AkU32>(AkBufferPool::INVALID);
        ft->free_pool.entries[free_pool_slot] = AkFrameId(entry_id);
        if (free_pool_slot == 0)
            break;
        entry_id++;
        free_pool_slot--;
    }
    ft->free_pool.count = aligned;
}

AkVoid frametable_fini(AkFrameTable *ft, AkAllocTable *at) noexcept {
    storage_framepool_fini(&ft->keep_pool, at);
    storage_framepool_fini(&ft->recycle_pool, at);
    storage_framepool_fini(&ft->default_pool, at);
    storage_framepool_fini(&ft->free_pool, at);
}

AkVoid storage_frametable_dump_debug(const AkFrameTable *ft) noexcept {
    std::print("FrameTable\n");
    std::print("  Pools\n");
    std::print("    Free pool size: {}\n", ft->free_pool.count);
    for (AkU32 i = 0; i < ft->free_pool.count; ++i) {
        std::print("      frame_id: {}\n", ft->free_pool.entries[i].id);
    }
    std::print("    Default pool size: {}\n", ft->default_pool.count);
    for (AkU32 i = 0; i < ft->default_pool.count; ++i) {
        std::print("      frame_id: {}\n", ft->default_pool.entries[i].id);
    }
    std::print("    Keep pool size: {}\n", ft->keep_pool.count);
    for (AkU32 i = 0; i < ft->keep_pool.count; ++i) {
        std::print("      frame_id: {}\n", ft->keep_pool.entries[i].id);
    }
    std::print("    Recycle pool size: {}\n", ft->recycle_pool.count);
    for (AkU32 i = 0; i < ft->recycle_pool.count; ++i) {
        std::print("      frame_id: {}\n", ft->recycle_pool.entries[i].id);
    }
    std::print("  Entries\n");
    AkU32 cap = storage_frametable_get_capacity(ft);
    for (AkU32 i = 0; i < cap; ++i) {
        const AkFrameEntry &e = ft->entries[i];
        AkFrameId frame_id(i);
        if (e.pool == static_cast<AkU32>(AkBufferPool::INVALID)) {
            std::print("    {: >5} | free\n", frame_id.id);
            continue;
        }
        std::print("    {: >5} | {: >8} -> is_dirty: {} | evict:{} | pins: {} | pool_index: {} | p_bucket: {} | "
                   "vp_bucket: {}\n",
                   frame_id.id, ak_to_string(static_cast<AkBufferPool>(e.pool)), (AkBool)e.is_dirty, (AkBool)e.evict,
                   (AkU32)e.pin_count, (AkU32)e.pool_index.id, (AkU32)e.page_cache_bucket.id, (AkU32)e.vpage_cache_bucket.id);
    }
}

AkFrameId storage_frametable_allocate_frame(AkFrameTable *ft, AkBufferPool pool) noexcept {
    AK_ASSERT(pool != AkBufferPool::INVALID);
    AK_ASSERT(ft->free_pool.count != 0);

    AkFramePool *target_pool = get_pool(ft, pool);

    ft->free_pool.count--;
    AkFrameId frame_id = ft->free_pool.entries[ft->free_pool.count];
    ft->free_pool.entries[ft->free_pool.count] = AkFrameId();

    AkU32 pool_index = target_pool->count;
    target_pool->entries[pool_index] = frame_id;
    target_pool->count++;

    AkFrameEntry &entry = ft->entries[frame_id.id];
    entry.pool = static_cast<AkU32>(pool);
    entry.pool_index = AkFrameId(pool_index);
    AK_ASSERT(entry.page_cache_bucket == AkPageId());
    AK_ASSERT(entry.vpage_cache_bucket == VPageId(std::numeric_limits<AkU32>::max()));

    return frame_id;
}

AkVoid storage_frametable_freeframe(AkFrameTable *ft, AkFrameId frame_id) noexcept {
    AkFrameEntry *frame_entry = get_entry(ft, frame_id);

    AK_ASSERT(frame_entry->evict != 0);
    AK_ASSERT(frame_entry->pin_count == 0);
    AK_ASSERT(frame_entry->is_dirty == 0);
    AK_ASSERT(frame_entry->page_cache_bucket == AkPageId());
    AK_ASSERT(frame_entry->vpage_cache_bucket == VPageId(std::numeric_limits<AkU32>::max()));

    AkFramePool *src_pool = get_pool(ft, static_cast<AkBufferPool>(frame_entry->pool));
    AK_ASSERT(src_pool->count > 0);

    AkFramePool *dest_pool = &ft->free_pool;
    AK_ASSERT(dest_pool->count < dest_pool->capacity);

    AkU32 src_pool_index = frame_entry->pool_index.id;
    AK_ASSERT(src_pool->entries[src_pool_index] == frame_id);

    if (src_pool_index != src_pool->count - 1) {
        AkFrameId last_entry_id = src_pool->entries[src_pool->count - 1];
        AkFrameEntry &last_entry = ft->entries[last_entry_id.id];
        last_entry.pool_index = AkFrameId(src_pool_index);
        src_pool->entries[src_pool_index] = last_entry_id;
    }
    src_pool->entries[src_pool->count] = AkFrameId();
    src_pool->count--;

    dest_pool->entries[dest_pool->count] = frame_id;
    frame_entry->pool_index = AkFrameId(dest_pool->count);
    frame_entry->pool = static_cast<AkU32>(AkBufferPool::INVALID);
    frame_entry->evict = 0;
    dest_pool->count++;
}

AkU32 storage_frametable_get_capacity(const AkFrameTable *ft) noexcept { return ft->free_pool.capacity; }

AkU32 storage_frametable_get_free_count(const AkFrameTable *ft) noexcept { return ft->free_pool.count; }

AkVoid storage_frametable_validate_frame_id(const AkFrameTable *ft, AkFrameId frame_id) noexcept { AK_ASSERT(frame_id.id < storage_frametable_get_capacity(ft)); }

AkVoid storage_frametable_move_to_pool(AkFrameTable *ft, AkFrameId frame_id, AkBufferPool dest_pool_type) noexcept {
    AK_ASSERT(frame_id.id < storage_frametable_get_capacity(ft));
    AK_ASSERT(dest_pool_type != AkBufferPool::INVALID);

    AkFrameEntry &entry = ft->entries[frame_id.id];
    if (entry.pool == static_cast<AkU32>(dest_pool_type))
        return;

    AkFramePool *src_pool = get_pool(ft, static_cast<AkBufferPool>(entry.pool));
    AK_ASSERT(src_pool->count > 0);

    AkFramePool *dest_pool = get_pool(ft, dest_pool_type);
    AK_ASSERT(dest_pool->count < dest_pool->capacity);

    AkU32 src_pool_index = entry.pool_index.id;
    AK_ASSERT(src_pool->entries[src_pool_index] == frame_id);

    if (src_pool_index != src_pool->count - 1) {
        AkFrameId last_entry_id = src_pool->entries[src_pool->count - 1];
        AkFrameEntry &last_entry = ft->entries[last_entry_id.id];
        last_entry.pool_index = AkFrameId(src_pool_index);
        src_pool->entries[src_pool_index] = last_entry_id;
    }
    src_pool->entries[src_pool->count] = AkFrameId();
    src_pool->count--;

    dest_pool->entries[dest_pool->count] = frame_id;
    entry.pool_index = AkFrameId(dest_pool->count);
    entry.pool = static_cast<AkU32>(dest_pool_type);
    dest_pool->count++;
}

AkVoid storage_framepool_free_check_invariants(const AkFrameTable *ft) noexcept {
    for (AkU32 idx = 0; idx < ft->free_pool.count; ++idx) {
        AkFrameId frame_id = ft->free_pool.entries[idx];
        const AkFrameEntry &e = ft->entries[frame_id.id];
        AK_ASSERT(e.pool == static_cast<AkU32>(AkBufferPool::INVALID));
        AK_ASSERT(e.pool_index.id == idx);
    }
}

AkVoid storage_framepool_keep_check_invariants(const AkFrameTable *ft) noexcept {
    for (AkU32 idx = 0; idx < ft->keep_pool.count; ++idx) {
        AkFrameId frame_id = ft->keep_pool.entries[idx];
        const AkFrameEntry &e = ft->entries[frame_id.id];
        AK_ASSERT(e.pool == static_cast<AkU32>(AkBufferPool::KEEP));
        AK_ASSERT(e.pool_index.id == idx);
    }
}

AkVoid storage_framepool_recycle_check_invariants(const AkFrameTable *ft) noexcept {
    for (AkU32 idx = 0; idx < ft->recycle_pool.count; ++idx) {
        AkFrameId frame_id = ft->recycle_pool.entries[idx];
        const AkFrameEntry &e = ft->entries[frame_id.id];
        AK_ASSERT(e.pool == static_cast<AkU32>(AkBufferPool::RECYCLE));
        AK_ASSERT(e.pool_index.id == idx);
    }
}

AkVoid storage_framepool_check_invariants(const AkFrameTable *ft) noexcept {
    AkU32 capacity = storage_frametable_get_capacity(ft);
    AkU32 pools_sum = ft->default_pool.count + ft->free_pool.count + ft->keep_pool.count + ft->recycle_pool.count;
    AK_ASSERT(capacity == pools_sum);
}

 // namespace ak
