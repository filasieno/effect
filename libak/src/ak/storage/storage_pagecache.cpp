#include "ak/storage/storage_api_priv.hpp"


// inline PagecacheEntry* get_pagecache_bucket_at(const Pagecache* cache, AkU32 bucket_id) noexcept;
// inline AkBool            is_pagecache_bucket_used(const Pagecache* cache, AkU32 bucket_index) noexcept;
inline static AkU32  hash(AkPageId id) noexcept;
inline static AkU32  hash(const AkPagecacheEntry& entry) noexcept;
inline static AkBool is_free(const AkPagecacheEntry& entry) noexcept;
inline static AkBool is_used(const AkPagecacheEntry& entry) noexcept;
inline static AkVoid clear(AkPagecacheEntry& entry) noexcept;

// PageCache page_cache_create(AllocTable* at, AkU32 capacity) noexcept {    
//     AkU32 aligned_capacity = 1;
//     while (aligned_capacity < capacity) aligned_capacity *= 2;
//     PageCacheEntry* entries = static_cast<PageCacheEntry*>(ak::alloc(at, aligned_capacity * sizeof(PageCacheEntry), alignof(PageCacheEntry)));
//     if (entries) {
//         for (AkU32 i = 0; i < aligned_capacity; ++i) {
//             page_cache_entry_clear(entries[i]);
//         }
//     }
//     return {entries, aligned_capacity};
// }

// AkVoid page_cache_destroy(PageCache& cache, AllocTable* at) noexcept {
//     if (cache.entries) {
//         ak::dealloc(at, cache.entries);
//         cache.entries = nullptr;
//         cache.capacity = 0;
//     }
// }


AkBool storage_pagecache_contains_entry(const AkPagecache* cache, AkPageId page_id) noexcept {
    AK_ASSERT(cache != nullptr);
    AK_ASSERT(page_id);
    
    if (cache->capacity == 0) return false;
    AkU32 h = hash(page_id);
    AkU32 mask = cache->capacity - 1;
    AkU32 entry_id = h & mask;
    while (true) {
        const auto& entry = cache->entries[entry_id];
        if (is_free(entry)) return false;
        if (entry.page_id == page_id) return true;
        entry_id = (entry_id + 1) & mask;
    }
    return false;
}

AkFrameId storage_pagecache_lookup_entry(const AkPagecache* cache, AkPageId page_id) noexcept {
    AK_ASSERT(cache != nullptr);
    if (cache->capacity == 0) return {};
    AkU32 h = hash(page_id);
    AkU32 mask = cache->capacity - 1;
    AkU32 entry_id = h & mask;
    while (true) {
        const auto& entry = cache->entries[entry_id];
        if (is_free(entry)) return {};
        if (entry.page_id == page_id) return entry.frame_id;
        entry_id = (entry_id + 1) & mask;
    }
    return {};
}

AkU32 storage_pagecache_put_entry(AkPagecache* cache, AkPageId page_id, AkFrameId frame_id) noexcept {
    AK_ASSERT(cache != nullptr);
    AK_ASSERT(cache->capacity > 0, "Cache not initialized");
    AkU32 h = hash(page_id);
    AkU32 mask = cache->capacity - 1;
    AkU32 entry_id = h & mask;
    while (true) {
        auto& entry = cache->entries[entry_id];
        if (is_free(entry)) {
            entry.page_id = page_id;
            entry.frame_id = frame_id;
            return entry_id;
        }
        if (entry.page_id == page_id) {
            entry.frame_id = frame_id;
            return entry_id;
        }
        entry_id = (entry_id + 1) & mask;
    }
    AK_ASSERT(false, "Cache full");
    return 0;
}


static AkVoid remove_and_update_hash_chain(AkPagecache* cache, AkU32 bucket_id) noexcept {
    AK_ASSERT(bucket_id < cache->capacity, "Invalid bucket_id");
    AkU32 j = bucket_id;
    AkU32 i = bucket_id;
    AkU32 mask = cache->capacity - 1;
    while (true) {
        j = (j + 1) & mask;
        auto& entry = cache->entries[j];
        if (is_free(entry)) break;
        AkU32 k = hash(entry) & mask;
        if ((j > i && (k <= i || k > j)) || (j < i && (k <= i && k > j))) {
            cache->entries[i] = entry;
            i = j;
            clear(entry);
        }
    }
    clear(cache->entries[i]);
}

    
AkFrameId storage_pagecache_remove_entry(AkPagecache* cache, AkPageId page_id) noexcept {
    AK_ASSERT(cache != nullptr);
    AK_ASSERT(page_id);
    if (cache->capacity == 0) return {};
    AkU32 h = hash(page_id);
    AkU32 mask = cache->capacity - 1;
    AkU32 entry_id = h & mask;
    while (true) {
        auto& entry = cache->entries[entry_id];
        if (is_free(entry)) return AkFrameId(AkFrameId::INVALID);
        if (entry.page_id == page_id) {
            AkFrameId out_frame_id = entry.frame_id;
            remove_and_update_hash_chain(cache, entry_id);
            return out_frame_id;
        }
        entry_id = (entry_id + 1) & mask;
    }
    return AkFrameId(AkFrameId::INVALID);
}

// Utilities 

inline static AkU32 hash(AkPageId id) noexcept {
    AK_ASSERT(id);
    AkU32 h = id.id;
    h ^= h >> 16;
    return h;
}

inline static AkU32 hash(const AkPagecacheEntry& entry) noexcept {
    return hash(entry.page_id);
}

inline static AkBool is_free(const AkPagecacheEntry& entry) noexcept {
    return entry.page_id == AkPageId::INVALID;
}

inline static AkBool is_used(const AkPagecacheEntry& entry) noexcept {
    return !is_free(entry);
}

inline static AkVoid clear(AkPagecacheEntry& entry) noexcept {
    entry.page_id = AkPageId(AkPageId::INVALID);
    entry.frame_id = AkFrameId(AkFrameId::INVALID);
}

inline AkPagecacheEntry* get_pagecache_bucket_at(const AkPagecache* cache, AkU32 bucket_id) noexcept {
    AK_ASSERT(cache != nullptr);
    AK_ASSERT(bucket_id < cache->capacity, "Invalid bucket_id");
    auto& entry = cache->entries[bucket_id];
    AK_ASSERT(! entry.page_id, "Invalid entry");
    return &entry;
}

inline AkBool is_pagecache_bucket_used(const AkPagecache* cache, AkU32 bucket_index) noexcept {
    AK_ASSERT(cache != nullptr);
    AK_ASSERT(bucket_index < cache->capacity, "Invalid bucket_index");
    return is_used(cache->entries[bucket_index]);
}

