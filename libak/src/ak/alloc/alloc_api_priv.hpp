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
    AkI32   init_alloc_table(AkAllocTable* at, AkVoid* mem, AkSize size) noexcept;
    AkVoid* try_alloc_table_malloc(AkAllocTable* at, AkSize size) noexcept;
    AkVoid  alloc_table_free(AkAllocTable* at, AkVoid* ptr, AkU32 side_coalescing) noexcept;
    AkI32   defrag_alloc_table_mem(AkAllocTable* at, AkU64 millis_budget) noexcept;
    AkVoid  check_alloc_table_invariants(AkAllocTable* at, std::source_location loc = std::source_location::current()) noexcept;
    AkI64   coalesce_alloc_table_right(AkAllocTable* at, AkAllocBlockHeader** out_block, AkU32 max_merges) noexcept;
    AkI64   coalesce_alloc_table_left(AkAllocTable* at, AkAllocBlockHeader** out_block, AkU32 max_merges) noexcept;

    // Free block Tree
    AkVoid                init_free_block_tree_root(AkAllocFreeBlockHeader** root) noexcept;
    AkVoid                put_free_block(AkAllocFreeBlockHeader** root, AkAllocBlockHeader* block) noexcept;
    AkAllocFreeBlockHeader* find_gte_free_block(AkAllocFreeBlockHeader* root, AkU64 block_size) noexcept;
    AkVoid                detach_free_block(AkAllocFreeBlockHeader** root, AkAllocFreeBlockHeader* node) noexcept;
    AkBool                is_detached(const AkAllocFreeBlockHeader* link) noexcept;
    AkVoid                clear(AkAllocFreeBlockHeader* link) noexcept;
    
    // Freeblock list bitmask utilities
    AkVoid set_alloc_freelist_mask(AkU64* bit_field, AkU64 bin_idx) noexcept;
    AkBool get_alloc_freelist_mask(const AkU64* bit_field, AkU64 bin_idx) noexcept;
    AkVoid clear_alloc_freelist_mask(AkU64* bit_field, AkU64 bin_idx) noexcept;
    AkI32  find_alloc_freelist_index(const AkU64* bit_field, AkSize alloc_size) noexcept;
    AkU32  get_alloc_freelist_index(const AkAllocBlockHeader* header) noexcept;
    AkU64  get_alloc_freelist_index(AkU64 sz) noexcept;

    
    // Iteration
    AkAllocBlockHeader* next(AkAllocBlockHeader* header) noexcept;
    AkAllocBlockHeader* prev(AkAllocBlockHeader* header) noexcept;    
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



