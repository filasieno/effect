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
AkI32   alloc_table_init(AkAllocTable* at, AkVoid* mem, AkSize size) noexcept;
AkVoid* alloc_table_try_malloc(AkAllocTable* at, AkSize size) noexcept;
AkVoid  alloc_table_free(AkAllocTable* at, AkVoid* ptr, AkU32 side_coalescing) noexcept;
AkI32   alloc_table_defrag(AkAllocTable* at, AkU64 millis_budget) noexcept;
AkVoid  alloc_table_check_invariants(AkAllocTable* at, std::source_location loc = std::source_location::current()) noexcept;
AkI64   alloc_table_coalesce_right(AkAllocTable* at, AkAllocBlockHeader** out_block, AkU32 max_merges) noexcept;
AkI64   alloc_table_coalesce_left(AkAllocTable* at, AkAllocBlockHeader** out_block, AkU32 max_merges) noexcept;

// Free block Tree
AkVoid                  alloc_freeblock_init_root(AkAllocFreeBlockHeader** root) noexcept;
AkVoid                  alloc_freeblock_put(AkAllocFreeBlockHeader** root, AkAllocBlockHeader* block) noexcept;
AkAllocFreeBlockHeader* alloc_freeblock_find_gte(AkAllocFreeBlockHeader* root, AkU64 block_size) noexcept;
AkVoid                  alloc_freeblock_detach(AkAllocFreeBlockHeader** root, AkAllocFreeBlockHeader* node) noexcept;
AkBool                  alloc_freeblock_is_detached(const AkAllocFreeBlockHeader* link) noexcept;
AkVoid                  alloc_freeblock_clear(AkAllocFreeBlockHeader* link) noexcept;

// Freeblock list bitmask utilities
AkVoid alloc_freelist_set_mask(AkU64* bit_field, AkU64 bin_idx) noexcept;
AkBool alloc_freelist_get_mask(const AkU64* bit_field, AkU64 bin_idx) noexcept;
AkVoid alloc_freelist_clear_mask(AkU64* bit_field, AkU64 bin_idx) noexcept;
AkI32  alloc_freelist_find_index(const AkU64* bit_field, AkSize alloc_size) noexcept;
AkU32  alloc_freelist_get_index(const AkAllocBlockHeader* header) noexcept;
AkU64  alloc_freelist_get_index(AkU64 size) noexcept;

// Block Iteration
AkAllocBlockHeader* alloc_block_next(AkAllocBlockHeader* header) noexcept;
AkAllocBlockHeader* alloc_block_prev(AkAllocBlockHeader* header) noexcept;    



