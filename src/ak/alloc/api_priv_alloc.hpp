#pragma once

#include "ak/api_priv.hpp" // IWYU pragma: keep


namespace ak { namespace priv {
    // alloc_freelist utilities
    // 64-bit freelist utilities
    Void set_alloc_freelist_mask(U64* bit_field, U64 bin_idx) noexcept;
    Bool get_alloc_freelist_mask(const U64* bit_field, U64 bin_idx) noexcept;
    Void clear_alloc_freelist_mask(U64* bit_field, U64 bin_idx) noexcept;
    U32  find_alloc_freelist_index(const U64* bit_field, Size alloc_size) noexcept;
    U32  get_alloc_freelist_index(const AllocBlockHeader* header) noexcept;
    

}}