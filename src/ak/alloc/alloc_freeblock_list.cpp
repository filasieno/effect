#include "ak/alloc/alloc.hpp" // IWYU pragma: keep
#include <cstdlib>

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
    I32 find_alloc_freelist_index(const U64* bit_field, Size alloc_size) noexcept {
        AK_ASSERT(bit_field != nullptr);
        // If no bins are populated, signal not found
        const U64 word = *bit_field;
        if (word == 0ull) return -1;
        // Requests larger than the max small bin size are not eligible for small freelists
        if (alloc_size > 2048ull) return -1;
        // Map size to bin: bin = ceil(size/32) - 1, clamped to [0,63]
        U64 required_bin = 0ull;
        if (alloc_size != 0) required_bin = (U64)((alloc_size - 1u) >> 5);
        if (required_bin > 63ull) required_bin = 63ull;
        const U64 mask = (~0ull) << required_bin;
        const U64 value = word & mask;
        if (value == 0ull) return -1; // no suitable bin found
        return (I32)__builtin_ctzll(value);
    }


    Void set_alloc_freelist_mask(U64* bit_field, U64 bin_idx) noexcept {
        AK_ASSERT(bit_field != nullptr);
        AK_ASSERT(bin_idx < 64);
        *bit_field |= (1ull << bin_idx);
    }

    Bool get_alloc_freelist_mask(const U64* bit_field, U64 bin_idx) noexcept {
        AK_ASSERT(bit_field != nullptr);
        AK_ASSERT(bin_idx < 64);
        return ((*bit_field >> bin_idx) & 1ull) != 0ull;
    }

    Void clear_alloc_freelist_mask(U64* bit_field, U64 bin_idx) noexcept {
        AK_ASSERT(bit_field != nullptr);
        AK_ASSERT(bin_idx < 64);
        *bit_field &= ~(1ull << bin_idx);
    }

    AllocBlockHeader* next(AllocBlockHeader* header) noexcept {
        size_t sz = (size_t)header->this_desc.size;
        if (sz == 0) return header;
        return (AllocBlockHeader*)((Char*)header + sz);
    }

    AllocBlockHeader* prev(AllocBlockHeader* header) noexcept {
        size_t sz = (size_t)header->prev_desc.size;
        if (sz == 0) return header;
        return (AllocBlockHeader*)((Char*)header - sz);
    }


    U64 get_alloc_freelist_index(U64 sz) noexcept {
        // New mapping: 0..32 -> 0, 33..64 -> 1, ..., up to 2048 -> 63
        AK_ASSERT(sz > 0);
        U64 bin = (U64)((sz - 1ull) >> 5);
        const U64 mask = (U64)-(bin > 63u);
        bin = (bin & ~mask) | (63ull & mask);
        return bin;
    }

    U32 get_alloc_freelist_index(const AllocBlockHeader* header) noexcept {
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
