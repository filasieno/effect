#pragma once

// Private API declaration
// ------------------------------------------------
// Defines the private API of the Allocator module.
// The contents of this file are part of the private API and WILL NOT be part of the public API

#include "ak/base/base_api.hpp"        // IWYU pragma: keep
#include "ak/alloc/alloc_api.hpp"      // IWYU pragma: keep

#include <source_location>

namespace ak { namespace priv {

    // Allocator Table
    I32   init_alloc_table(AllocTable* at, Void* mem, Size size) noexcept;
    Void* try_alloc_table_malloc(AllocTable* at, Size size) noexcept;
    Void  alloc_table_free(AllocTable* at, Void* ptr, U32 side_coalescing) noexcept;
    I32   defrag_alloc_table_mem(AllocTable* at, U64 millis_budget) noexcept;
    Void  check_alloc_table_invariants(AllocTable* at, std::source_location loc = std::source_location::current()) noexcept;
    I64   coalesce_alloc_table_right(AllocTable* at, AllocBlockHeader** out_block, U32 max_merges) noexcept;
    I64   coalesce_alloc_table_left(AllocTable* at, AllocBlockHeader** out_block, U32 max_merges) noexcept;

    // Free block Tree
    Void                  init_free_block_tree_root(AllocFreeBlockHeader** root) noexcept;
    Void                  put_free_block(AllocFreeBlockHeader** root, AllocBlockHeader* block) noexcept;
    AllocFreeBlockHeader* find_gte_free_block(AllocFreeBlockHeader* root, U64 block_size) noexcept;
    Void                  detach_free_block(AllocFreeBlockHeader** root, AllocFreeBlockHeader* node) noexcept;
    Bool                  is_detached(const AllocFreeBlockHeader* link) noexcept;
    Void                  clear(AllocFreeBlockHeader* link) noexcept;
    
    // Freeblock list bitmask utilities
    Void set_alloc_freelist_mask(U64* bit_field, U64 bin_idx) noexcept;
    Bool get_alloc_freelist_mask(const U64* bit_field, U64 bin_idx) noexcept;
    Void clear_alloc_freelist_mask(U64* bit_field, U64 bin_idx) noexcept;
    I32  find_alloc_freelist_index(const U64* bit_field, Size alloc_size) noexcept;
    U32  get_alloc_freelist_index(const AllocBlockHeader* header) noexcept;
    U64 get_alloc_freelist_index(U64 sz) noexcept;

    
    // Iteration
    AllocBlockHeader* next(AllocBlockHeader* header) noexcept;
    AllocBlockHeader* prev(AllocBlockHeader* header) noexcept;    
}}

// Allocator convenience wrappers used by runtime
namespace ak {
    Void* try_alloc_mem(Size sz) noexcept;
    Void  free_mem(Void* ptr, U32 side_coalesching = UINT_MAX) noexcept;
    I32   defragment_mem(U64 millis_time_budget = ~0ull) noexcept;
}

