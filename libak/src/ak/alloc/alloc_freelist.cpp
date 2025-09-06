#include "ak/alloc/alloc.hpp" // IWYU pragma: keep
#include <cstdlib>

// Private Allocator API implementation
// ----------------------------------------------------------------------------------------------------------------
    
/// \brief Find the smallest free list that can store the alloc_size
/// 
/// \param alloc_size The size of the allocation
/// \param bit_field A pointer to a 64 byte aligned bit field
/// \return The index of the smallest free list that can store the alloc_size
/// \pre AVX2 is available
/// \pre bitField is 64 byte aligned
/// \internal
int alloc_freelist_find_index(const AkU64* bit_field, AkSize alloc_size) noexcept {
    AK_ASSERT(bit_field != nullptr);
    // If no bins are populated, signal not found
    const AkU64 word = *bit_field;
    if (word == 0ull) return -1;
    // Requests larger than the max small bin size are not eligible for small freelists
    if (alloc_size > 2048ull) return -1;
    // Map size to bin: bin = ceil(size/32) - 1, clamped to [0,63]
    AkU64 required_bin = 0ull;
    if (alloc_size != 0) required_bin = (AkU64)((alloc_size - 1u) >> 5);
    if (required_bin > 63ull) required_bin = 63ull;
    const AkU64 mask = (~0ull) << required_bin;
    const AkU64 value = word & mask;
    if (value == 0ull) return -1; // no suitable bin found
    return (int)__builtin_ctzll(value);
}


void alloc_freelist_set_mask(AkU64* bit_field, AkU64 bin_idx) noexcept {
    AK_ASSERT(bit_field != nullptr);
    AK_ASSERT(bin_idx < 64);
    *bit_field |= (1ull << bin_idx);
}

bool alloc_freelist_get_mask(const AkU64* bit_field, AkU64 bin_idx) noexcept {
    AK_ASSERT(bit_field != nullptr);
    AK_ASSERT(bin_idx < 64);
    return ((*bit_field >> bin_idx) & 1ull) != 0ull;
}

void alloc_freelist_clear_mask(AkU64* bit_field, AkU64 bin_idx) noexcept {
    AK_ASSERT(bit_field != nullptr);
    AK_ASSERT(bin_idx < 64);
    *bit_field &= ~(1ull << bin_idx);
}

ak_alloc_block_header* alloc_block_next(ak_alloc_block_header* header) noexcept {
    size_t sz = (size_t)header->this_desc.size;
    if (sz == 0) return header;
    return (ak_alloc_block_header*)((char*)header + sz);
}

ak_alloc_block_header* alloc_block_prev(ak_alloc_block_header* header) noexcept {
    size_t sz = (size_t)header->prev_desc.size;
    if (sz == 0) return header;
    return (ak_alloc_block_header*)((char*)header - sz);
}


AkU64 alloc_freelist_get_index(AkU64 sz) noexcept {
    // New mapping: 0..32 -> 0, 33..64 -> 1, ..., up to 2048 -> 63
    AK_ASSERT(sz > 0);
    AkU64 bin = (AkU64)((sz - 1ull) >> 5);
    const AkU64 mask = (AkU64)-(bin > 63u);
    bin = (bin & ~mask) | (63ull & mask);
    return bin;
}

AkU32 alloc_freelist_get_index(const ak_alloc_block_header* header) noexcept {
    switch ((enum ak_alloc_block_state)header->this_desc.state) {
        case AK_ALLOC_BLOCK_STATE_WILD_BLOCK:
            return 63;
        case AK_ALLOC_BLOCK_STATE_FREE: 
        {
            const AkU64 sz = header->this_desc.size;
            AkU64 bin = (AkU64)((sz - 1ull) >> 5);
            const AkU64 mask = (AkU64)-(bin > 63u);
            bin = (bin & ~mask) | (63u & mask);
            return bin;
        }
        case AK_ALLOC_BLOCK_STATE_INVALID:
        case AK_ALLOC_BLOCK_STATE_USED:
        case AK_ALLOC_BLOCK_STATE_BEGIN_SENTINEL:
        case AK_ALLOC_BLOCK_STATE_END_SENTINEL:
        default:
        {
            // Unreachable
            std::abort();
            return (AkU32)~0;
        }
    }
}
