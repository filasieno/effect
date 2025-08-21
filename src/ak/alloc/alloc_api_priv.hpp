#pragma once

// Private API declaration
// ------------------------------------------------
// Defines the private API of the Allocator module.
// The contents of this file are part of the private API and WILL NOT be part of the public API

#include "ak/base/base_api.hpp"        // IWYU pragma: keep
#include "ak/alloc/alloc_api.hpp"      // IWYU pragma: keep

namespace ak { namespace priv {
    Void check_alloc_table_invariants(const std::source_location loc = std::source_location::current()) noexcept;

    // Free block header Tree
    Void                  init_free_block_tree_root(AllocFreeBlockHeader** root) noexcept;
    Void                  put_free_block(AllocFreeBlockHeader** root, AllocBlockHeader* block) noexcept;
    AllocFreeBlockHeader* find_gte_free_block(AllocFreeBlockHeader* root, U64 block_size) noexcept;
    Void                  detach_free_block(AllocFreeBlockHeader** root, AllocFreeBlockHeader* node) noexcept;
    Bool                  is_detached(const AllocFreeBlockHeader* link) noexcept;
    Void                  clear(AllocFreeBlockHeader* link) noexcept;
    
    // alloc_freelist utilities
    Void set_alloc_freelist_mask(U64* bit_field, U64 bin_idx) noexcept;
    Bool get_alloc_freelist_mask(const U64* bit_field, U64 bin_idx) noexcept;
    Void clear_alloc_freelist_mask(U64* bit_field, U64 bin_idx) noexcept;
    I32  find_alloc_freelist_index(const U64* bit_field, Size alloc_size) noexcept;
    U32  get_alloc_freelist_index(const AllocBlockHeader* header) noexcept;

    // Coaleshing
    I64 coalesce_right(AllocBlockHeader** out_block, U32 max_merges) noexcept;
    I64 coalesce_left(AllocBlockHeader** out_block, U32 max_merges) noexcept;

    // Iteration
    AllocBlockHeader* next(AllocBlockHeader* header) noexcept;
    AllocBlockHeader* prev(AllocBlockHeader* header) noexcept;    
}}

