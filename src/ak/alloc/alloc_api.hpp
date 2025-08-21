#pragma once

#include "ak/base/base_api.hpp" // base types/macros

namespace ak {

    enum class AllocBlockState {
        INVALID              = 0b0000,
        USED                 = 0b0010,
        FREE                 = 0b0001,
        WILD_BLOCK           = 0b0011,
        BEGIN_SENTINEL       = 0b0100,
        LARGE_BLOCK_SENTINEL = 0b0110,
        END_SENTINEL         = 0b1100,
    };
    const Char* to_string(AllocBlockState) noexcept;

    enum class AllocKind {
        INVALID = 0,
        GENERIC_MALLOC,
        PROMISE,
        FREE_SEGMENT_INDEX_LEAF,
        FREE_SEGMENT_INDEX_INNER,
        FREE_SEGMENT_INDEX_LEAF_EXTENSION
    };

    struct AllocBlockDesc { U64 size:48; U64 state:4; U64 kind:12; };
    struct AllocBlockHeader { AllocBlockDesc this_desc; AllocBlockDesc prev_desc; };
    struct AllocPooledFreeBlockHeader : public AllocBlockHeader { priv::DLink freelist_link; };
    static_assert(sizeof(AllocPooledFreeBlockHeader) == 32);

    struct AllocFreeBlockHeader : public AllocBlockHeader {
        priv::DLink            multimap_link;
        AllocFreeBlockHeader*  parent;
        AllocFreeBlockHeader*  left;
        AllocFreeBlockHeader*  right;
        I32                    height;
        I32                    balance;
    };
    static_assert(sizeof(AllocFreeBlockHeader) == 64, "AllocFreeBlockHeader size is not 64 bytes");

    struct AllocStats {
        static constexpr int ALLOCATOR_BIN_COUNT = 64;
        static constexpr int STATS_BIN_COUNT = 66;
        static constexpr int STATS_IDX_TREE = 64;
        static constexpr int STATS_IDX_WILD = 65;
        Size alloc_counter[STATS_BIN_COUNT];
        Size realloc_counter[STATS_BIN_COUNT];
        Size free_counter[STATS_BIN_COUNT];
        Size failed_counter[STATS_BIN_COUNT];
        Size split_counter[STATS_BIN_COUNT];
        Size merged_counter[STATS_BIN_COUNT];
        Size reused_counter[STATS_BIN_COUNT];
        Size pooled_counter[STATS_BIN_COUNT];
    };

    struct AllocTable {
        static constexpr int ALLOCATOR_BIN_COUNT = AllocStats::ALLOCATOR_BIN_COUNT;
        alignas(8)  U64         freelist_mask;
        alignas(64) priv::DLink freelist_head[ALLOCATOR_BIN_COUNT];
        alignas(64) U32         freelist_count[ALLOCATOR_BIN_COUNT];
        alignas(8) Char* heap_begin;
        alignas(8) Char* heap_end;
        alignas(8) Char* mem_begin;
        alignas(8) Char* mem_end;
        Size mem_size;
        Size free_mem_size;
        Size max_free_block_size;
        AllocStats stats;
        alignas(8) AllocPooledFreeBlockHeader* sentinel_begin;
        alignas(8) AllocPooledFreeBlockHeader* sentinel_end;
        alignas(8) AllocPooledFreeBlockHeader* wild_block;
        alignas(8) AllocFreeBlockHeader*       root_free_block;
    };

    namespace priv {
        constexpr U64 ALLOC_STATE_IS_USED_MASK     = 0;
        constexpr U64 ALLOC_STATE_IS_FREE_MASK     = 1;
        constexpr U64 ALLOC_STATE_IS_SENTINEL_MASK = 4;
    }

} // namespace ak

