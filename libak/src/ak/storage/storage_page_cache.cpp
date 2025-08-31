#include "ak/storage/storage_page_cache_priv.hpp"

namespace ak {

    // inline PagecacheEntry* get_pagecache_bucket_at(const Pagecache* cache, AkU32 bucket_id) noexcept;
    // inline AkBool            is_pagecache_bucket_used(const Pagecache* cache, AkU32 bucket_index) noexcept;
    inline static AkU32  hash(PageId id) noexcept;
    inline static AkU32  hash(const PagecacheEntry& entry) noexcept;
    inline static AkBool is_free(const PagecacheEntry& entry) noexcept;
    inline static AkBool is_used(const PagecacheEntry& entry) noexcept;
    inline static AkVoid clear(PagecacheEntry& entry) noexcept;

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


    AkBool contains_pagecache_entry(const Pagecache* cache, PageId page_id) noexcept {
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

    FrameId lookup_pagecahe_entry(const Pagecache* cache, PageId page_id) noexcept {
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

    AkU32 put_pagecache_entry(Pagecache* cache, PageId page_id, FrameId frame_id) noexcept {
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

    namespace {
        void remove_and_update_hash_chain(Pagecache* cache, AkU32 bucket_id) noexcept {
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
    } // namespace
        
    FrameId remove_pagecache_entry(Pagecache* cache, PageId page_id) noexcept {
        AK_ASSERT(cache != nullptr);
        AK_ASSERT(page_id);
        if (cache->capacity == 0) return {};
        AkU32 h = hash(page_id);
        AkU32 mask = cache->capacity - 1;
        AkU32 entry_id = h & mask;
        while (true) {
            auto& entry = cache->entries[entry_id];
            if (is_free(entry)) return FrameId(FrameId::INVALID);
            if (entry.page_id == page_id) {
                FrameId out_frame_id = entry.frame_id;
                remove_and_update_hash_chain(cache, entry_id);
                return out_frame_id;
            }
            entry_id = (entry_id + 1) & mask;
        }
        return FrameId(FrameId::INVALID);
    }

    // Utilities 

    inline static AkU32 hash(PageId id) noexcept {
        AK_ASSERT(id);
        AkU32 h = id.id;
        h ^= h >> 16;
        return h;
    }

    inline static AkU32 hash(const PagecacheEntry& entry) noexcept {
        return hash(entry.page_id);
    }

    inline static AkBool is_free(const PagecacheEntry& entry) noexcept {
        return entry.page_id == PageId::INVALID;
    }

    inline static AkBool is_used(const PagecacheEntry& entry) noexcept {
        return !is_free(entry);
    }

    inline static void clear(PagecacheEntry& entry) noexcept {
        entry.page_id = PageId(PageId::INVALID);
        entry.frame_id = FrameId(FrameId::INVALID);
    }

    inline PagecacheEntry* get_pagecache_bucket_at(const Pagecache* cache, AkU32 bucket_id) noexcept {
        AK_ASSERT(cache != nullptr);
        AK_ASSERT(bucket_id < cache->capacity, "Invalid bucket_id");
        auto& entry = cache->entries[bucket_id];
        AK_ASSERT(! entry.page_id, "Invalid entry");
        return &entry;
    }

    inline AkBool is_pagecache_bucket_used(const Pagecache* cache, AkU32 bucket_index) noexcept {
        AK_ASSERT(cache != nullptr);
        AK_ASSERT(bucket_index < cache->capacity, "Invalid bucket_index");
        return is_used(cache->entries[bucket_index]);
    }


} // namespace ak
