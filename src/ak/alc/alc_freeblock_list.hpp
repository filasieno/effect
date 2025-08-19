#pragma once

#include "ak/alc/alc_api_priv.hpp" // IWYU pragma: keep

// Private Allocator API implementation
// ----------------------------------------------------------------------------------------------------------------
namespace ak { namespace priv {
   
    
    /// \brief Find the smallest free list that can store the alloc_size
    /// 
    /// \param alloc_size The size of the allocation
    /// \param bit_field A pointer to a 64 byte aligned bit field
    /// \return The index of the smallest free list that can store the alloc_size
    /// \pre AVX2 is available
    /// \pre bitField is 64 byte aligned
    /// \internal
    inline U32 find_alloc_freelist_index(const U64* bit_field, Size alloc_size) noexcept {
        assert(bit_field != nullptr);
        // Map size to bin: bin = ceil(size/32) - 1, clamped to [0,63]; bin 63 means > 2016 and <= 2048
        U64 required_bin = 0ull;
        if (alloc_size != 0) required_bin = (U64)((alloc_size - 1u) >> 5);
        if (required_bin > 63ull) required_bin = 63ull;
        U64 word = *bit_field;
        U64 mask = (~0ull) << required_bin;
        U64 value = word & mask;
        if (value == 0ull) return 63; // no exact/greater small bin; 63 used as boundary (medium/wild)
        return (U32)__builtin_ctzll(value);
    }

    

    inline Void set_alloc_freelist_mask(U64* bit_field, U64 bin_idx) noexcept {
        assert(bit_field != nullptr);
        assert(bin_idx < 64);
        *bit_field |= (1ull << bin_idx);
    }

    inline Bool get_alloc_freelist_mask(const U64* bit_field, U64 bin_idx) noexcept {
        assert(bit_field != nullptr);
        assert(bin_idx < 64);
        return ((*bit_field >> bin_idx) & 1ull) != 0ull;
    }

    inline Void clear_alloc_freelist_mask(U64* bit_field, U64 bin_idx) noexcept {
        assert(bit_field != nullptr);
        assert(bin_idx < 64);
        *bit_field &= ~(1ull << bin_idx);
    }

    inline AllocBlockHeader* next(AllocBlockHeader* header) noexcept {
        size_t sz = (size_t)header->this_desc.size;
        if (sz == 0) return header;
        return (AllocBlockHeader*)((Char*)header + sz);
    }

    inline AllocBlockHeader* prev(AllocBlockHeader* header) noexcept {
        size_t sz = (size_t)header->prev_desc.size;
        if (sz == 0) return header;
        return (AllocBlockHeader*)((Char*)header - sz);
    }

    inline U64 get_alloc_freelist_index(U64 sz) noexcept {
        // New mapping: 0..32 -> 0, 33..64 -> 1, ..., up to 2048 -> 63
        assert(sz > 0);
        U64 bin = (U64)((sz - 1ull) >> 5);
        const U64 mask = (U64)-(bin > 63u);
        bin = (bin & ~mask) | (63ull & mask);
        return bin;
    }

    inline U32 get_alloc_freelist_index(const AllocBlockHeader* header) noexcept {
        switch ((AllocBlockState)header->this_desc.state) {
            case AllocBlockState::WILD_BLOCK:
                return 63;
            case AllocBlockState::FREE: 
            {
                const U64 sz = header->this_desc.size;
                U64 bin = (U64)((sz - 1ull) >> 5);
                const U64 mask = (U64)-(bin > 63u);
                bin = (bin & ~mask) | (63u & mask);
                return bin;
            }
            case AllocBlockState::INVALID:
            case AllocBlockState::USED:
            case AllocBlockState::BEGIN_SENTINEL:
            case AllocBlockState::END_SENTINEL:
            default:
            {
               // Unreachable
               std::abort();
               return UINT_MAX;
            }
        }
    }
}}
