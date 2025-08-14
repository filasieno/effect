#pragma once

#include "dlist.hpp"
#include "types.hpp"

namespace ak {

    namespace priv {

        enum class AllocState {
            INVALID              = 0b0000,
            USED                 = 0b0010,
            FREE                 = 0b0001,
            WILD_BLOCK           = 0b0011,
            BEGIN_SENTINEL       = 0b0100,
            LARGE_BLOCK_SENTINEL = 0b0110,
            END_SENTINEL         = 0b1100,
        };

        struct AllocSizeRecord {
            U64 size      : 48;
            U64 state     : 4;
            U64 _reserved : 12;
        };

        constexpr U64 ALLOC_STATE_IS_USED_MASK     = 0;
        constexpr U64 ALLOC_STATE_IS_FREE_MASK     = 1;
        constexpr U64 ALLOC_STATE_IS_SENTINEL_MASK = 4;    

        struct AllocHeader {
            AllocSizeRecord thisSize;
            AllocSizeRecord prevSize;
        };

        struct FreeAllocHeader {
            AllocSizeRecord thisSize;
            AllocSizeRecord prevSize;
            DLink freeListLink;
        };
        static_assert(sizeof(FreeAllocHeader) == 32);

        static constexpr int ALLOCATOR_BIN_COUNT = 256;

        struct AllocStats {
            Size binAllocCount[ALLOCATOR_BIN_COUNT];
            Size binReallocCount[ALLOCATOR_BIN_COUNT];
            Size binFreeCount[ALLOCATOR_BIN_COUNT];
            Size binSplitCount[ALLOCATOR_BIN_COUNT];
            Size binMergeCount[ALLOCATOR_BIN_COUNT];
            Size binReuseCount[ALLOCATOR_BIN_COUNT];
            Size binPoolCount[ALLOCATOR_BIN_COUNT];
        };

        struct AllocTable {
            // FREE LIST MANAGEMENT
            alignas(64) __m256i    freeListbinMask;                         
            alignas(64) utl::DLink freeListBins[ALLOCATOR_BIN_COUNT];
            alignas(64) U32        freeListBinsCount[ALLOCATOR_BIN_COUNT];

            // HEAP BOUNDARY MANAGEMENT
            alignas(8) char* heapBegin;
            alignas(8) char* heapEnd;
            alignas(8) char* memBegin;
            alignas(8) char* memEnd;
            
            // MEMORY ACCOUNTING
            Size memSize;
            Size usedMemSize;
            Size freeMemSize;
            Size maxFreeBlockSize;
            
            // ALLOCATION STATISTICS
            AllocStats stats;
            
            // SENTINEL BLOCKS
            alignas(8) FreeAllocHeader* beginSentinel;
            alignas(8) FreeAllocHeader* wildBlock;
            alignas(8) FreeAllocHeader* largeBlockSentinel;
            alignas(8) FreeAllocHeader* endSentinel;
        };

        int InitAllocTable(AllocTable* at, void* mem, Size size) noexcept;
    } // namespace priv

} // namespace ak

namespace ak 
{
    namespace priv 
    {
        
    }
}