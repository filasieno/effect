#pragma once

#include "ak/base/base_api.hpp" // base types/macros

enum ak_alloc_block_state 
{
    AK_ALLOC_BLOCK_STATE_INVALID              = 0b0000,
    AK_ALLOC_BLOCK_STATE_USED                 = 0b0010,
    AK_ALLOC_BLOCK_STATE_FREE                 = 0b0001,
    AK_ALLOC_BLOCK_STATE_WILD_BLOCK           = 0b0011,
    AK_ALLOC_BLOCK_STATE_BEGIN_SENTINEL       = 0b0100,
    AK_ALLOC_BLOCK_STATE_LARGE_BLOCK_SENTINEL = 0b0110,
    AK_ALLOC_BLOCK_STATE_END_SENTINEL         = 0b1100,
};
const char* to_string(enum ak_alloc_block_state) noexcept;

enum class AkAllocKind 
{
    INVALID = 0,
    GENERIC_MALLOC,
    PROMISE,
    FREE_SEGMENT_INDEX_LEAF,
    FREE_SEGMENT_INDEX_INNER,
    FREE_SEGMENT_INDEX_LEAF_EXTENSION
};

struct ak_alloc_block_desc 
{ 
    AkU64 size:48; 
    AkU64 state:4; 
    AkU64 kind:12; 
};

struct ak_alloc_block_header 
{ 
    struct ak_alloc_block_desc this_desc; 
    struct ak_alloc_block_desc prev_desc; 
};

struct AkAllocPooledFreeBlockHeader : public ak_alloc_block_header 
{ 
    ak_dlink freelist_link; 
};
static_assert(sizeof(AkAllocPooledFreeBlockHeader) == 32);

struct ak_alloc_free_block_header : public ak_alloc_block_header 
{
    struct ak_dlink                    multimap_link;
    struct ak_alloc_free_block_header* parent;
    struct ak_alloc_free_block_header* left;
    struct ak_alloc_free_block_header* right;
    int                                height;
    int                                balance;
};
static_assert(sizeof(ak_alloc_free_block_header) == 64, "AllocFreeBlockHeader size is not 64 bytes");

struct ak_alloc_stats 
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


struct ak_alloc_table 
{
    static constexpr int ALLOCATOR_BIN_COUNT = ak_alloc_stats::ALLOCATOR_BIN_COUNT;

    alignas(8)  AkU64                         freelist_mask;
    alignas(64) struct ak_dlink               freelist_head[ALLOCATOR_BIN_COUNT];
    alignas(64) AkU32                         freelist_count[ALLOCATOR_BIN_COUNT];
    alignas(8)  char*                         heap_begin;
    alignas(8)  char*                         heap_end;
    alignas(8)  char*                         mem_begin;
    alignas(8)  char*                         mem_end;
    alignas(8)  AkSize                        mem_size;
    alignas(8)  AkSize                        free_mem_size;
    alignas(8)  AkSize                        max_free_block_size;
    alignas(8)  ak_alloc_stats                stats;
    alignas(8)  AkAllocPooledFreeBlockHeader* sentinel_begin;
    alignas(8)  AkAllocPooledFreeBlockHeader* sentinel_end;
    alignas(8)  AkAllocPooledFreeBlockHeader* wild_block;
    alignas(8)  ak_alloc_free_block_header*   root_free_block;
};



