#pragma once

#include "ak/api_priv.hpp" // IWYU pragma: keep


namespace ak { namespace priv {
    // alloc_freelist utilities
    using Vec256i = __m256i;
    
    Void set_alloc_freelist_mask(Vec256i* bit_field, U64 bin_idx) noexcept;
    Bool get_alloc_freelist_mask(Vec256i* bit_field, U64 bin_idx) noexcept;
    Void clear_alloc_freelist_mask(Vec256i* bit_field, U64 bin_idx) noexcept;
    U32  find_alloc_freelist_index(Vec256i* bit_field, Size alloc_size) noexcept;
    U32  get_alloc_freelist_index(const AllocBlockHeader* header) noexcept;
    

}}