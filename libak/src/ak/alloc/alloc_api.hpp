#pragma once

#include "ak/base/base_api.hpp" // base types/macros

enum class AkAllocBlockState 
{
    INVALID              = 0b0000,
    USED                 = 0b0010,
    FREE                 = 0b0001,
    WILD_BLOCK           = 0b0011,
    BEGIN_SENTINEL       = 0b0100,
    LARGE_BLOCK_SENTINEL = 0b0110,
    END_SENTINEL         = 0b1100,
};
const AkChar* to_string(AkAllocBlockState) noexcept;

enum class AkAllocKind 
{
    INVALID = 0,
    GENERIC_MALLOC,
    PROMISE,
    FREE_SEGMENT_INDEX_LEAF,
    FREE_SEGMENT_INDEX_INNER,
    FREE_SEGMENT_INDEX_LEAF_EXTENSION
};

struct AkAllocBlockDesc 
{ 
    AkU64 size:48; 
    AkU64 state:4; 
    AkU64 kind:12; 
};

struct AkAllocBlockHeader 
{ 
    AkAllocBlockDesc this_desc; 
    AkAllocBlockDesc prev_desc; 
};

struct AkAllocPooledFreeBlockHeader : public AkAllocBlockHeader 
{ 
    AkDLink freelist_link; 
};
static_assert(sizeof(AkAllocPooledFreeBlockHeader) == 32);

struct AkAllocFreeBlockHeader : public AkAllocBlockHeader 
{
    AkDLink                 multimap_link;
    AkAllocFreeBlockHeader* parent;
    AkAllocFreeBlockHeader* left;
    AkAllocFreeBlockHeader* right;
    AkI32                   height;
    AkI32                   balance;
};
static_assert(sizeof(AkAllocFreeBlockHeader) == 64, "AllocFreeBlockHeader size is not 64 bytes");

struct AkAllocStats 
{
    static constexpr int ALLOCATOR_BIN_COUNT = 64;
    static constexpr int STATS_BIN_COUNT = 66;
    static constexpr int STATS_IDX_TREE = 64;
    static constexpr int STATS_IDX_WILD = 65;

    AkSize alloc_counter[STATS_BIN_COUNT];
    AkSize realloc_counter[STATS_BIN_COUNT];
    AkSize free_counter[STATS_BIN_COUNT];
    AkSize failed_counter[STATS_BIN_COUNT];
    AkSize split_counter[STATS_BIN_COUNT];
    AkSize merged_counter[STATS_BIN_COUNT];
    AkSize reused_counter[STATS_BIN_COUNT];
    AkSize pooled_counter[STATS_BIN_COUNT];
};


struct AkAllocTable 
{
    static constexpr int ALLOCATOR_BIN_COUNT = AkAllocStats::ALLOCATOR_BIN_COUNT;

    alignas(8)  AkU64                         freelist_mask;
    alignas(64) AkDLink                       freelist_head[ALLOCATOR_BIN_COUNT];
    alignas(64) AkU32                         freelist_count[ALLOCATOR_BIN_COUNT];
    alignas(8)  AkChar*                       heap_begin;
    alignas(8)  AkChar*                       heap_end;
    alignas(8)  AkChar*                       mem_begin;
    alignas(8)  AkChar*                       mem_end;
    alignas(8)  AkSize                        mem_size;
    alignas(8)  AkSize                        free_mem_size;
    alignas(8)  AkSize                        max_free_block_size;
    alignas(8)  AkAllocStats                  stats;
    alignas(8)  AkAllocPooledFreeBlockHeader* sentinel_begin;
    alignas(8)  AkAllocPooledFreeBlockHeader* sentinel_end;
    alignas(8)  AkAllocPooledFreeBlockHeader* wild_block;
    alignas(8)  AkAllocFreeBlockHeader*       root_free_block;
};



