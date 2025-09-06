#include "ak/storage/storage_api_priv.hpp" // IWYU pragma: keep
#include <bit>
#include <print>
#include <algorithm> // for std::fill

inline static ak_framepool *get_pool(ak_frametable *ft, enum ak_bufferpool pool) noexcept {
    switch (pool) {
    case AK_BUFFERPOOL_INVALID:
        return &ft->free_pool;
    case AK_BUFFERPOOL_DEFAULT:
        return &ft->default_pool;
    case AK_BUFFERPOOL_RECYCLE:
        return &ft->recycle_pool;
    case AK_BUFFERPOOL_KEEP:
        return &ft->keep_pool;
    }
    std::abort();
    // Unreachable
    return nullptr;
}

inline static ak_framepool_entry *get_entry(ak_frametable *ft, ak_frame_id frame_id) noexcept {
    storage_frametable_validate_frame_id(ft, frame_id);
    return &ft->entries[frame_id.id];
}

void storage_framepool_init(ak_framepool *fp, AkU32 capacity, struct ak_alloc_table *at) noexcept {
    AkU32 aligned_capacity = std::bit_ceil(static_cast<AkU32>(capacity));
    AkSize byte_size = static_cast<AkSize>(aligned_capacity) * sizeof(ak_frame_id);
    ak_frame_id *e = static_cast<ak_frame_id *>(alloc_table_try_malloc(at, byte_size));
    AK_ASSERT(e != nullptr);
    std::fill(e, e + aligned_capacity, ak_frame_id());
    fp->entries = e;
    fp->count = 0;
    fp->capacity = aligned_capacity;
}

void storage_framepool_fini(ak_framepool *fp, struct ak_alloc_table *at) noexcept {
    alloc_table_free(at, fp->entries, 0);
    fp->entries = nullptr;
    fp->count = 0;
    fp->capacity = 0;
}

bool storage_framepool_is_full(const ak_framepool *fp) noexcept { return fp->count == fp->capacity; }

void frametable_init(ak_frametable *ft, AkU32 capacity, struct ak_alloc_table *at) noexcept {

    AkU32 aligned = std::bit_ceil(static_cast<AkU32>(capacity));
    AkSize byte_size = static_cast<AkSize>(aligned) * sizeof(ak_framepool_entry);
    ak_framepool_entry *entries = static_cast<ak_framepool_entry *>(alloc_table_try_malloc(at, byte_size));
    AK_ASSERT(entries != nullptr);

    storage_framepool_init(&ft->free_pool, aligned, at);
    storage_framepool_init(&ft->default_pool, aligned, at);
    storage_framepool_init(&ft->recycle_pool, aligned, at);
    storage_framepool_init(&ft->keep_pool, aligned, at);

    AkU32 entry_id = 0;
    AkU32 free_pool_slot = aligned - 1;
    while (true) {
        ak_framepool_entry *entry = &entries[entry_id];
        *entry = ak_framepool_entry();
        entry->pool_index = ak_frame_id(free_pool_slot);
        entry->pool = (unsigned)AK_BUFFERPOOL_INVALID;
        ft->free_pool.entries[free_pool_slot] = ak_frame_id(entry_id);
        if (free_pool_slot == 0)
            break;
        entry_id++;
        free_pool_slot--;
    }
    ft->free_pool.count = aligned;
}

void frametable_fini(ak_frametable *ft, struct ak_alloc_table *at) noexcept {
    storage_framepool_fini(&ft->keep_pool, at);
    storage_framepool_fini(&ft->recycle_pool, at);
    storage_framepool_fini(&ft->default_pool, at);
    storage_framepool_fini(&ft->free_pool, at);
}

void storage_frametable_dump_debug(const ak_frametable *ft) noexcept {
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
        const ak_framepool_entry &e = ft->entries[i];
        ak_frame_id frame_id(i);
        if (e.pool == static_cast<AkU32>(AK_BUFFERPOOL_INVALID)) {
            std::print("    {: >5} | free\n", frame_id.id);
            continue;
        }
        std::print("    {: >5} | {: >8} -> is_dirty: {} | evict:{} | pins: {} | pool_index: {} | p_bucket: {} | "
                   "vp_bucket: {}\n",
                   frame_id.id, ak_to_string(static_cast<ak_bufferpool>(e.pool)), (bool)e.is_dirty, (bool)e.evict, (AkU32)e.pin_count, (AkU32)e.pool_index.id, (AkU32)e.page_cache_bucket.id, (AkU32)e.vpage_cache_bucket.id);
    }
}

ak_frame_id storage_frametable_allocate_frame(ak_frametable *ft, ak_bufferpool pool) noexcept {
    AK_ASSERT(pool != AK_BUFFERPOOL_INVALID);
    AK_ASSERT(ft->free_pool.count != 0);

    ak_framepool *target_pool = get_pool(ft, pool);

    ft->free_pool.count--;
    ak_frame_id frame_id = ft->free_pool.entries[ft->free_pool.count];
    ft->free_pool.entries[ft->free_pool.count] = ak_frame_id();

    AkU32 pool_index = target_pool->count;
    target_pool->entries[pool_index] = frame_id;
    target_pool->count++;

    ak_framepool_entry &entry = ft->entries[frame_id.id];
    entry.pool = static_cast<AkU32>(pool);
    entry.pool_index = ak_frame_id(pool_index);
    AK_ASSERT(entry.page_cache_bucket == ak_page_id());
    AK_ASSERT(entry.vpage_cache_bucket == ak_vpage_id(std::numeric_limits<AkU32>::max()));

    return frame_id;
}

void storage_frametable_freeframe(ak_frametable *ft, ak_frame_id frame_id) noexcept {
    ak_framepool_entry *frame_entry = get_entry(ft, frame_id);

    AK_ASSERT(frame_entry->evict != 0);
    AK_ASSERT(frame_entry->pin_count == 0);
    AK_ASSERT(frame_entry->is_dirty == 0);
    AK_ASSERT(frame_entry->page_cache_bucket == ak_page_id());
    AK_ASSERT(frame_entry->vpage_cache_bucket == ak_vpage_id(std::numeric_limits<AkU32>::max()));

    ak_framepool *src_pool = get_pool(ft, static_cast<ak_bufferpool>(frame_entry->pool));
    AK_ASSERT(src_pool->count > 0);

    ak_framepool *dest_pool = &ft->free_pool;
    AK_ASSERT(dest_pool->count < dest_pool->capacity);

    AkU32 src_pool_index = frame_entry->pool_index.id;
    AK_ASSERT(src_pool->entries[src_pool_index] == frame_id);

    if (src_pool_index != src_pool->count - 1) {
        ak_frame_id last_entry_id = src_pool->entries[src_pool->count - 1];
        ak_framepool_entry &last_entry = ft->entries[last_entry_id.id];
        last_entry.pool_index = ak_frame_id(src_pool_index);
        src_pool->entries[src_pool_index] = last_entry_id;
    }
    src_pool->entries[src_pool->count] = ak_frame_id();
    src_pool->count--;

    dest_pool->entries[dest_pool->count] = frame_id;
    frame_entry->pool_index = ak_frame_id(dest_pool->count);
    frame_entry->pool = static_cast<AkU32>(AK_BUFFERPOOL_INVALID);
    frame_entry->evict = 0;
    dest_pool->count++;
}

AkU32 storage_frametable_get_capacity(const ak_frametable *ft) noexcept { return ft->free_pool.capacity; }

AkU32 storage_frametable_get_free_count(const ak_frametable *ft) noexcept { return ft->free_pool.count; }

void storage_frametable_validate_frame_id(const ak_frametable *ft, ak_frame_id frame_id) noexcept { AK_ASSERT(frame_id.id < storage_frametable_get_capacity(ft)); }

void storage_frametable_move_to_pool(ak_frametable *ft, ak_frame_id frame_id, ak_bufferpool dest_pool_type) noexcept {
    AK_ASSERT(frame_id.id < storage_frametable_get_capacity(ft));
    AK_ASSERT(dest_pool_type != AK_BUFFERPOOL_INVALID);

    ak_framepool_entry &entry = ft->entries[frame_id.id];
    if (entry.pool == static_cast<AkU32>(dest_pool_type))
        return;

    ak_framepool *src_pool = get_pool(ft, static_cast<ak_bufferpool>(entry.pool));
    AK_ASSERT(src_pool->count > 0);

    ak_framepool *dest_pool = get_pool(ft, dest_pool_type);
    AK_ASSERT(dest_pool->count < dest_pool->capacity);

    AkU32 src_pool_index = entry.pool_index.id;
    AK_ASSERT(src_pool->entries[src_pool_index] == frame_id);

    if (src_pool_index != src_pool->count - 1) {
        ak_frame_id last_entry_id = src_pool->entries[src_pool->count - 1];
        ak_framepool_entry &last_entry = ft->entries[last_entry_id.id];
        last_entry.pool_index = ak_frame_id(src_pool_index);
        src_pool->entries[src_pool_index] = last_entry_id;
    }
    src_pool->entries[src_pool->count] = ak_frame_id();
    src_pool->count--;

    dest_pool->entries[dest_pool->count] = frame_id;
    entry.pool_index = ak_frame_id(dest_pool->count);
    entry.pool = static_cast<AkU32>(dest_pool_type);
    dest_pool->count++;
}

void storage_framepool_free_check_invariants(const ak_frametable *ft) noexcept {
    for (AkU32 idx = 0; idx < ft->free_pool.count; ++idx) {
        ak_frame_id frame_id = ft->free_pool.entries[idx];
        const ak_framepool_entry &e = ft->entries[frame_id.id];
        AK_ASSERT(e.pool == static_cast<AkU32>(AK_BUFFERPOOL_INVALID));
        AK_ASSERT(e.pool_index.id == idx);
    }
}

void storage_framepool_keep_check_invariants(const ak_frametable *ft) noexcept {
    for (AkU32 idx = 0; idx < ft->keep_pool.count; ++idx) {
        ak_frame_id frame_id = ft->keep_pool.entries[idx];
        const ak_framepool_entry &e = ft->entries[frame_id.id];
        AK_ASSERT(e.pool == static_cast<AkU32>(AK_BUFFERPOOL_KEEP));
        AK_ASSERT(e.pool_index.id == idx);
    }
}

void storage_framepool_recycle_check_invariants(const ak_frametable *ft) noexcept {
    for (AkU32 idx = 0; idx < ft->recycle_pool.count; ++idx) {
        ak_frame_id frame_id = ft->recycle_pool.entries[idx];
        const ak_framepool_entry &e = ft->entries[frame_id.id];
        AK_ASSERT(e.pool == static_cast<AkU32>(AK_BUFFERPOOL_RECYCLE));
        AK_ASSERT(e.pool_index.id == idx);
    }
}

void storage_framepool_check_invariants(const ak_frametable *ft) noexcept {
    AkU32 capacity = storage_frametable_get_capacity(ft);
    AkU32 pools_sum = ft->default_pool.count + ft->free_pool.count + ft->keep_pool.count + ft->recycle_pool.count;
    AK_ASSERT(capacity == pools_sum);
}

// namespace ak
