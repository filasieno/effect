#pragma once

// Private API declaration
// ------------------------------------------------
// Defines the private API of the Allocator module.
// The contents of this file are part of the private API and WILL NOT be part of the public API

#include "ak/base/base_api.hpp"        // IWYU pragma: keep
#include "ak/alloc/alloc_api.hpp"      // IWYU pragma: keep

#include <source_location>

namespace ak::priv {

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
    AkVoid alloc_set_freelist_mask(AkU64* bit_field, AkU64 bin_idx) noexcept;
    AkBool alloc_get_freelist_mask(const AkU64* bit_field, AkU64 bin_idx) noexcept;
    AkVoid alloc_clear_freelist_mask(AkU64* bit_field, AkU64 bin_idx) noexcept;
    AkI32  alloc_find_freelist_index(const AkU64* bit_field, AkSize alloc_size) noexcept;
    AkU32  alloc_get_freelist_index(const AkAllocBlockHeader* header) noexcept;
    AkU64  alloc_get_freelist_index(AkU64 sz) noexcept;

    
    // Block Iteration
    AkAllocBlockHeader* alloc_next_block(AkAllocBlockHeader* header) noexcept;
    AkAllocBlockHeader* alloc_prev_block(AkAllocBlockHeader* header) noexcept;    
}


namespace ak {

    namespace priv 
    {
        constexpr AkU64 ALLOC_STATE_IS_USED_MASK     = 0;
        constexpr AkU64 ALLOC_STATE_IS_FREE_MASK     = 1;
        constexpr AkU64 ALLOC_STATE_IS_SENTINEL_MASK = 4;
    }

} // namespace ak

// Allocator convenience wrappers used by runtime



