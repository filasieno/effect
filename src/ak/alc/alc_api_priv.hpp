#pragma once

#include "ak/api_priv.hpp" // IWYU pragma: keep


namespace ak { namespace priv {
    Void check_alloc_table_invariants(const std::source_location loc = std::source_location::current()) noexcept;
    
    // alloc_freelist utilities
    // 64-bit freelist utilities
    Void set_alloc_freelist_mask(U64* bit_field, U64 bin_idx) noexcept;
    Bool get_alloc_freelist_mask(const U64* bit_field, U64 bin_idx) noexcept;
    Void clear_alloc_freelist_mask(U64* bit_field, U64 bin_idx) noexcept;
    I32  find_alloc_freelist_index(const U64* bit_field, Size alloc_size) noexcept;
    U32  get_alloc_freelist_index(const AllocBlockHeader* header) noexcept;

    // Block Headers
    AllocBlockHeader* next(AllocBlockHeader* header) noexcept;
    AllocBlockHeader* prev(AllocBlockHeader* header) noexcept;

    I64 coalesce_right(AllocBlockHeader** out_block, U32 max_merges) noexcept;
    I64 coalesce_left(AllocBlockHeader** out_block, U32 max_merges) noexcept;
    
}}