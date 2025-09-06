#include "ak/storage/storage_api_priv.hpp"


// inline PagecacheEntry* get_pagecache_bucket_at(const Pagecache* cache, AkU32 bucket_id) noexcept;
// inline bool            is_pagecache_bucket_used(const Pagecache* cache, AkU32 bucket_index) noexcept;
inline static AkU32  hash(ak_page_id id) noexcept;
inline static AkU32  hash(const ak_pagecahe_entry& entry) noexcept;
inline static bool is_free(const ak_pagecahe_entry& entry) noexcept;
inline static bool is_used(const ak_pagecahe_entry& entry) noexcept;
inline static void clear(ak_pagecahe_entry& entry) noexcept;

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

// void page_cache_destroy(PageCache& cache, AllocTable* at) noexcept {
//     if (cache.entries) {
//         ak::dealloc(at, cache.entries);
//         cache.entries = nullptr;
//         cache.capacity = 0;
//     }
// }


bool storage_pagecache_contains_entry(const ak_pagecache* cache, ak_page_id page_id) noexcept {
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

ak_frame_id storage_pagecache_lookup_entry(const ak_pagecache* cache, ak_page_id page_id) noexcept {
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

AkU32 storage_pagecache_put_entry(ak_pagecache* cache, ak_page_id page_id, ak_frame_id frame_id) noexcept {
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


static void remove_and_update_hash_chain(ak_pagecache* cache, AkU32 bucket_id) noexcept {
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

    
ak_frame_id storage_pagecache_remove_entry(ak_pagecache* cache, ak_page_id page_id) noexcept {
    AK_ASSERT(cache != nullptr);
    AK_ASSERT(page_id);
    if (cache->capacity == 0) return {};
    AkU32 h = hash(page_id);
    AkU32 mask = cache->capacity - 1;
    AkU32 entry_id = h & mask;
    while (true) {
        auto& entry = cache->entries[entry_id];
        if (is_free(entry)) return ak_frame_id(ak_frame_id::INVALID);
        if (entry.page_id == page_id) {
            ak_frame_id out_frame_id = entry.frame_id;
            remove_and_update_hash_chain(cache, entry_id);
            return out_frame_id;
        }
        entry_id = (entry_id + 1) & mask;
    }
    return ak_frame_id(ak_frame_id::INVALID);
}

// Utilities 

inline static AkU32 hash(ak_page_id id) noexcept {
    AK_ASSERT(id);
    AkU32 h = id.id;
    h ^= h >> 16;
    return h;
}

inline static AkU32 hash(const ak_pagecahe_entry& entry) noexcept {
    return hash(entry.page_id);
}

inline static bool is_free(const ak_pagecahe_entry& entry) noexcept {
    return entry.page_id == ak_page_id::INVALID;
}

inline static bool is_used(const ak_pagecahe_entry& entry) noexcept {
    return !is_free(entry);
}

inline static void clear(ak_pagecahe_entry& entry) noexcept {
    entry.page_id = ak_page_id(ak_page_id::INVALID);
    entry.frame_id = ak_frame_id(ak_frame_id::INVALID);
}

inline ak_pagecahe_entry* get_pagecache_bucket_at(const ak_pagecache* cache, AkU32 bucket_id) noexcept {
    AK_ASSERT(cache != nullptr);
    AK_ASSERT(bucket_id < cache->capacity, "Invalid bucket_id");
    auto& entry = cache->entries[bucket_id];
    AK_ASSERT(! entry.page_id, "Invalid entry");
    return &entry;
}

inline bool is_pagecache_bucket_used(const ak_pagecache* cache, AkU32 bucket_index) noexcept {
    AK_ASSERT(cache != nullptr);
    AK_ASSERT(bucket_index < cache->capacity, "Invalid bucket_index");
    return is_used(cache->entries[bucket_index]);
}

