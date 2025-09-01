#pragma once

#include "ak/storage/storage_api.hpp" // IWYU pragma: keep
#include "ak/alloc/alloc.hpp"         // IWYU pragma: keep

// Frametable
AkVoid        storage_frametable_init(AkFrameTable* ft, AkU32 capacity, AkAllocTable* at) noexcept;
AkVoid        storage_frametable_fini(AkFrameTable* ft, AkAllocTable* at) noexcept;
AkVoid        storage_frametable_dump_debug(const AkFrameTable* ft) noexcept;
AkFrameId     storage_frametable_allocate_frame(AkFrameTable* ft, AkBufferPool pool) noexcept;
AkVoid        storage_frametable_freeframe(AkFrameTable* ft, AkFrameId frame_id) noexcept;
AkFrameEntry* storage_frametable_get_entry(AkFrameTable* ft, AkFrameId frame_id) noexcept;
AkU32         storage_frametable_get_capacity(const AkFrameTable* ft) noexcept;
AkU32         storage_frametable_get_free_count(const AkFrameTable* ft) noexcept;
AkVoid        storage_frametable_validate_frame_id(const AkFrameTable* ft, AkFrameId frame_id) noexcept;
AkVoid        storage_frametable_move_to_pool(AkFrameTable* ft, AkFrameId frame_id, AkBufferPool dest_pool_type) noexcept;

// FramePool
AkVoid        storage_framepool_init(AkFramePool* framePool, AkU32 capacity, AkAllocTable* at) noexcept;
AkVoid        storage_framepool_fini(AkFramePool* pool, AkAllocTable* at) noexcept;
AkBool        storage_framepool_is_full(const AkFramePool* pool) noexcept;
AkVoid        storage_framepool_free_check_invariants(const AkFrameTable* ft) noexcept;
AkVoid        storage_framepool_keep_check_invariants(const AkFrameTable* ft) noexcept;
AkVoid        storage_framepool_recycle_check_invariants(const AkFrameTable* ft) noexcept;
AkVoid        storage_framepool_check_invariants(const AkFrameTable* ft) noexcept;

// PageCache
AkPagecache   storage_pagecache_init(AkAllocTable* at, AkU32 capacity) noexcept;
AkVoid        storage_pagecache_fini(AkPagecache* cache, AkAllocTable* at) noexcept;
AkBool        storage_pagecache_contains_entry(const AkPagecache* cache, AkPageId page_id) noexcept;
AkFrameId     storage_pagecache_lookup_entry(const AkPagecache* cache, AkPageId page_id) noexcept;
AkU32         storage_pagecache_put_entry(AkPagecache* cache, AkPageId page_id, AkFrameId frame_id) noexcept;
AkFrameId     storage_pagecache_remove_entry(AkPagecache* cache, AkPageId page_id) noexcept;