#pragma once

// Private API declaration
// ------------------------------------------------
// Defines the private API of the Allocator module.
// The contents of this file are part of the private API and WILL NOT be part of the public API

#include "ak/base/base_api.hpp"        // IWYU pragma: keep
#include "ak/alloc/alloc_api.hpp"      // IWYU pragma: keep

#include <source_location>

constexpr AkU64 ALLOC_STATE_IS_USED_MASK     = 0;
constexpr AkU64 ALLOC_STATE_IS_FREE_MASK     = 1;
constexpr AkU64 ALLOC_STATE_IS_SENTINEL_MASK = 4;

// Allocator Table
int   alloc_table_init(struct ak_alloc_table* at, void* mem, AkSize size) noexcept;
void* alloc_table_try_malloc(struct ak_alloc_table* at, AkSize size) noexcept;
void  alloc_table_free(struct ak_alloc_table* at, void* ptr, AkU32 side_coalescing) noexcept;
int   alloc_table_defrag(struct ak_alloc_table* at, AkU64 millis_budget) noexcept;
void  alloc_table_check_invariants(struct ak_alloc_table* at, std::source_location loc = std::source_location::current()) noexcept;
AkI64 alloc_table_coalesce_right(struct ak_alloc_table* at, struct ak_alloc_block_header** out_block, AkU32 max_merges) noexcept;
AkI64 alloc_table_coalesce_left(struct ak_alloc_table* at, struct ak_alloc_block_header** out_block, AkU32 max_merges) noexcept;

// Free block Tree
void                               alloc_freeblock_init_root(struct ak_alloc_free_block_header** root) noexcept;
void                               alloc_freeblock_put(struct ak_alloc_free_block_header** root, struct ak_alloc_block_header* block) noexcept;
struct ak_alloc_free_block_header* alloc_freeblock_find_gte(struct ak_alloc_free_block_header* root, AkU64 block_size) noexcept;
void                               alloc_freeblock_detach(struct ak_alloc_free_block_header** root, struct ak_alloc_free_block_header* node) noexcept;
bool                               alloc_freeblock_is_detached(const struct ak_alloc_free_block_header* link) noexcept;
void                               alloc_freeblock_clear(struct ak_alloc_free_block_header* link) noexcept;

// Freeblock list bitmask utilities
void  alloc_freelist_set_mask(AkU64* bit_field, AkU64 bin_idx) noexcept;
bool  alloc_freelist_get_mask(const AkU64* bit_field, AkU64 bin_idx) noexcept;
void  alloc_freelist_clear_mask(AkU64* bit_field, AkU64 bin_idx) noexcept;
int   alloc_freelist_find_index(const AkU64* bit_field, AkSize alloc_size) noexcept;
AkU32 alloc_freelist_get_index(const struct ak_alloc_block_header* header) noexcept;
AkU64 alloc_freelist_get_index(AkU64 size) noexcept;

// Block Iteration
struct ak_alloc_block_header* alloc_block_next(struct ak_alloc_block_header* header) noexcept;
struct ak_alloc_block_header* alloc_block_prev(struct ak_alloc_block_header* header) noexcept;    



