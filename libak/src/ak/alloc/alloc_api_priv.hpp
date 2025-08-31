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
    AkI32   init_alloc_table(AllocTable* at, AkVoid* mem, AkSize size) noexcept;
    AkVoid* try_alloc_table_malloc(AllocTable* at, AkSize size) noexcept;
    AkVoid  alloc_table_free(AllocTable* at, AkVoid* ptr, AkU32 side_coalescing) noexcept;
    AkI32   defrag_alloc_table_mem(AllocTable* at, AkU64 millis_budget) noexcept;
    AkVoid  check_alloc_table_invariants(AllocTable* at, std::source_location loc = std::source_location::current()) noexcept;
    AkI64   coalesce_alloc_table_right(AllocTable* at, AllocBlockHeader** out_block, AkU32 max_merges) noexcept;
    AkI64   coalesce_alloc_table_left(AllocTable* at, AllocBlockHeader** out_block, AkU32 max_merges) noexcept;

    // Free block Tree
    AkVoid                init_free_block_tree_root(AllocFreeBlockHeader** root) noexcept;
    AkVoid                put_free_block(AllocFreeBlockHeader** root, AllocBlockHeader* block) noexcept;
    AllocFreeBlockHeader* find_gte_free_block(AllocFreeBlockHeader* root, AkU64 block_size) noexcept;
    AkVoid                detach_free_block(AllocFreeBlockHeader** root, AllocFreeBlockHeader* node) noexcept;
    AkBool                is_detached(const AllocFreeBlockHeader* link) noexcept;
    AkVoid                clear(AllocFreeBlockHeader* link) noexcept;
    
    // Freeblock list bitmask utilities
    AkVoid set_alloc_freelist_mask(AkU64* bit_field, AkU64 bin_idx) noexcept;
    AkBool get_alloc_freelist_mask(const AkU64* bit_field, AkU64 bin_idx) noexcept;
    AkVoid clear_alloc_freelist_mask(AkU64* bit_field, AkU64 bin_idx) noexcept;
    AkI32  find_alloc_freelist_index(const AkU64* bit_field, AkSize alloc_size) noexcept;
    AkU32  get_alloc_freelist_index(const AllocBlockHeader* header) noexcept;
    AkU64  get_alloc_freelist_index(AkU64 sz) noexcept;

    
    // Iteration
    AllocBlockHeader* next(AllocBlockHeader* header) noexcept;
    AllocBlockHeader* prev(AllocBlockHeader* header) noexcept;    
}

// Allocator convenience wrappers used by runtime
namespace ak {
    AkVoid* try_alloc_mem(AkSize sz) noexcept;
    AkVoid  free_mem(AkVoid* ptr, AkU32 side_coalesching = (AkU32)~0) noexcept;
    AkI32   defragment_mem(AkU64 millis_time_budget = ~0ull) noexcept;
}

