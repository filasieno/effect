#pragma once

#include "ak/storage/storage_api.hpp" // IWYU pragma: keep
#include "ak/alloc/alloc.hpp"         // IWYU pragma: keep

// Frametable
void storage_frametable_init(struct ak_frametable *ft, AkU32 capacity, ak_alloc_table *at) noexcept;
void storage_frametable_fini(struct ak_frametable *ft, ak_alloc_table *at) noexcept;
void storage_frametable_dump_debug(const struct ak_frametable *ft) noexcept;
ak_frame_id storage_frametable_allocate_frame(struct ak_frametable *ft, ak_bufferpool pool) noexcept;
void storage_frametable_freeframe(struct ak_frametable *ft, ak_frame_id frame_id) noexcept;
struct ak_framepool_entry *storage_frametable_get_entry(struct ak_frametable *ft, ak_frame_id frame_id) noexcept;
AkU32 storage_frametable_get_capacity(const struct ak_frametable *ft) noexcept;
AkU32 storage_frametable_get_free_count(const struct ak_frametable *ft) noexcept;
void storage_frametable_validate_frame_id(const struct ak_frametable *ft, ak_frame_id frame_id) noexcept;
void storage_frametable_move_to_pool(struct ak_frametable *ft, ak_frame_id frame_id, ak_bufferpool dest_pool_type) noexcept;

// FramePool
void storage_framepool_init(struct ak_framepool *framePool, AkU32 capacity, ak_alloc_table *at) noexcept;
void storage_framepool_fini(struct ak_framepool *pool, ak_alloc_table *at) noexcept;
bool storage_framepool_is_full(const struct ak_framepool *pool) noexcept;
void storage_framepool_free_check_invariants(const ak_frametable *ft) noexcept;
void storage_framepool_keep_check_invariants(const ak_frametable *ft) noexcept;
void storage_framepool_recycle_check_invariants(const ak_frametable *ft) noexcept;
void storage_framepool_check_invariants(const ak_frametable *ft) noexcept;

// PageCache
struct ak_pagecache *storage_pagecache_init(struct ak_alloc_table *at, AkU32 capacity) noexcept;
void storage_pagecache_fini(struct ak_pagecache *cache, struct ak_alloc_table *at) noexcept;
bool storage_pagecache_contains_entry(const struct ak_pagecache *cache, ak_page_id page_id) noexcept;
ak_frame_id storage_pagecache_lookup_entry(const struct ak_pagecache *cache, ak_page_id page_id) noexcept;
AkU32 storage_pagecache_put_entry(struct ak_pagecache *cache, ak_page_id page_id, ak_frame_id frame_id) noexcept;
ak_frame_id storage_pagecache_remove_entry(struct ak_pagecache *cache, ak_page_id page_id) noexcept;