#pragma once

#include "ak/api_priv.hpp"

namespace ak { namespace priv {
    // FreeBin utilities
    Void SetAllocFreeBinBit(__m256i* bitField, U64 binIdx) noexcept;
    Bool GetAllocFreeBinBit(__m256i* bitField, U64 binIdx) noexcept;
    Void ClearAllocFreeBinBit(__m256i* bitField, U64 binIdx) noexcept;
    int  FindAllocFreeListBinIndex(__m256i* bitField, Size allocSize) noexcept;


    unsigned GetAllocFreeListBinIndex(const AllocHeader* h) noexcept;
}}



// Private Allocator API implementation
// ----------------------------------------------------------------------------------------------------------------
namespace ak { namespace priv {
   
    
    /// \brief Find the smallest free list that can store the allocSize
    /// 
    /// \param allocSize The size of the allocation
    /// \param bitField A pointer to a 64 byte aligned bit field
    /// \return The index of the smallest free list that can store the allocSize
    /// \pre AVX2 is available
    /// \pre bitField is 64 byte aligned
    /// \internal
    inline int FindAllocFreeListBinIndex(__m256i* bitField, Size allocSize) noexcept {
        assert(bitField != nullptr);
        assert(((uintptr_t)bitField % 64ull) == 0ull);

        // Compute the starting bin index (ceil(allocSize/32) - 1), clamped to [0,255]
        unsigned requiredBin = 0u;
        if (allocSize != 0) {
            requiredBin = (unsigned)((allocSize - 1u) >> 5); // floor((allocSize-1)/32)
        }
        if (requiredBin > 255u) requiredBin = 255u;

        // Safely load as 4x64-bit words to avoid strict-aliasing pitfalls
        uint64_t words[4];
        std::memcpy(words, bitField, sizeof(words));

        const unsigned startingWordIndex = requiredBin >> 6; // 0..3
        const unsigned bitInWord         = requiredBin & 63u; // 0..63

        for (unsigned wi = startingWordIndex; wi < 4u; ++wi) {
            const uint64_t mask = (wi == startingWordIndex) ? (~0ull << bitInWord) : ~0ull;
            const uint64_t v = words[wi] & mask;
            if (v != 0ull) {
                return (int)(wi * 64u + (unsigned)__builtin_ctzll(v));
            }
        }
        // No small bin available at or after required -> use wild (255)
        return 255;
    }

    inline int InitAllocTable(Void* mem, Size size) noexcept {
        AllocTable* at = &gKernel.alloc_table;
        
        constexpr U64 SENTINEL_SIZE = sizeof(FreeAllocHeader);

        assert(mem != nullptr);
        assert(size >= 4096);

        memset((Void*)at, 0, sizeof(AllocTable));
        
        // Establish heap boundaries
        Char* heapBegin = (Char*)(mem);
        Char* heapEnd   = heapBegin + size;

        // // Align start up to 32 and end down to 32 to keep all blocks 32B-multiples
        U64 alignedBegin = ((U64)heapBegin + SENTINEL_SIZE) & ~31ull;
        U64 alignedEnd   = ((U64)heapEnd   - SENTINEL_SIZE) & ~31ull;

        at->heapBegin = heapBegin;
        at->heapEnd   = heapEnd;
        at->memBegin  = (Char*)alignedBegin;
        at->memEnd    = (Char*)alignedEnd;
        at->memSize   = (Size)(at->memEnd - at->memBegin);

        // Addresses
        // Layout: [BeginSentinel] ... blocks ... [LargeBlockSentinel] ... largeBlocks ... [EndSentinel]
        FreeAllocHeader* beginSentinel      = (FreeAllocHeader*)alignedBegin;
        FreeAllocHeader* wildBlock          = (FreeAllocHeader*)((Char*)beginSentinel + SENTINEL_SIZE);
        FreeAllocHeader* endSentinel        = (FreeAllocHeader*)((Char*)alignedEnd    - SENTINEL_SIZE); 
        FreeAllocHeader* largeBlockSentinel = (FreeAllocHeader*)((Char*)endSentinel   - SENTINEL_SIZE);
        utl::init_link(&wildBlock->freeListLink);
        
        // Check alignments
        assert(((U64)beginSentinel      & 31ull) == 0ull);
        assert(((U64)wildBlock          & 31ull) == 0ull);
        assert(((U64)endSentinel        & 31ull) == 0ull);
        assert(((U64)largeBlockSentinel & 31ull) == 0ull);
        
        at->beginSentinel      = beginSentinel;
        at->wildBlock          = wildBlock;
        at->endSentinel        = endSentinel;
        at->largeBlockSentinel = largeBlockSentinel;
        
        beginSentinel->thisSize.size       = (U64)SENTINEL_SIZE;
        beginSentinel->thisSize.state      = (U32)AllocState::BEGIN_SENTINEL;
        // Initialize prevSize for the begin sentinel to avoid reading
        // uninitialized memory in debug printers.
        beginSentinel->prevSize            = { 0ull, (U32)AllocState::INVALID, 0ull };
        wildBlock->thisSize.size           = (U64)((U64)largeBlockSentinel - (U64)wildBlock);
        wildBlock->thisSize.state          = (U32)AllocState::WILD_BLOCK;
        largeBlockSentinel->thisSize.size  = (U64)SENTINEL_SIZE;
        largeBlockSentinel->thisSize.state = (U32)AllocState::LARGE_BLOCK_SENTINEL;
        endSentinel->thisSize.size         = (U64)SENTINEL_SIZE;
        endSentinel->thisSize.state        = (U32)AllocState::END_SENTINEL;
        wildBlock->prevSize                = beginSentinel->thisSize;
        largeBlockSentinel->prevSize       = wildBlock->thisSize;
        endSentinel->prevSize              = largeBlockSentinel->thisSize;
        at->freeMemSize                    = wildBlock->thisSize.size;

        for (int i = 0; i < 255; ++i) { // Wild block (255) stays cleared
            utl::init_link(&at->freeListBins[i]);
        }
        at->freeListBinsCount[255] = 1; // Wild block has always exaclty one block
        SetAllocFreeBinBit(&at->freeListbinMask, 255);

        return 0;
    }

    inline Void SetAllocFreeBinBit(__m256i* bitField, U64 binIdx) noexcept {
        assert(bitField != nullptr);
        assert(binIdx < 256);
        const U64 lane = binIdx >> 6;  // 0..3
        const U64 bit  = binIdx & 63u; // 0..63
        uint64_t word;
        std::memcpy(&word, reinterpret_cast<const Char*>(bitField) + lane * sizeof(uint64_t), sizeof(uint64_t));
        word |= (1ull << bit);
        std::memcpy(reinterpret_cast<Char*>(bitField) + lane * sizeof(uint64_t), &word, sizeof(uint64_t));
    }

    inline Bool GetAllocFreeBinBit(__m256i* bitField, U64 binIdx) noexcept {
        assert(bitField != nullptr);
        assert(binIdx < 256);
        const U64 lane = binIdx >> 6;  // 0..3
        const U64 bit  = binIdx & 63u; // 0..63
        uint64_t word;
        std::memcpy(&word, reinterpret_cast<const Char*>(bitField) + lane * sizeof(uint64_t), sizeof(uint64_t));
        return ((word >> bit) & 1ull) != 0ull;
    }

    inline Void ClearAllocFreeBinBit(__m256i* bitField, U64 binIdx) noexcept {
        assert(bitField != nullptr);
        assert(binIdx < 256);
        const U64 lane = binIdx >> 6;  // 0..3
        const U64 bit  = binIdx & 63u; // 0..63
        uint64_t word;
        std::memcpy(&word, reinterpret_cast<const Char*>(bitField) + lane * sizeof(uint64_t), sizeof(uint64_t));
        word &= ~(1ull << bit);
        std::memcpy(reinterpret_cast<Char*>(bitField) + lane * sizeof(uint64_t), &word, sizeof(uint64_t));
    }

    inline AllocHeader* NextAllocHeaderPtr(AllocHeader* h) noexcept {
        size_t sz = (size_t)h->thisSize.size;
        if (sz == 0) return h;
        return (AllocHeader*)((Char*)h + sz);
    }

    inline AllocHeader* PrevAllocHeaderPtr(AllocHeader* h) noexcept {
        size_t sz = (size_t)h->prevSize.size;
        if (sz == 0) return h;
        return (AllocHeader*)((Char*)h - sz);
    }

    inline U64 GetAllocSmallBinIndexFromSize(U64 sz) noexcept {
        // Bin mapping: bin = ceil(sz/32) - 1, clamped to [0, 254]
        // Examples: 1..32 -> 0, 33..64 -> 1, ..., 8160 -> 254
        assert(sz > 0);
        U64 bin = (U64)((sz - 1ull) >> 5);
        const U64 mask = (U64)-(bin > 254u);
        bin = (bin & ~mask) | (254ull & mask);
        return bin;
    }

    inline unsigned GetAllocFreeListBinIndex(const AllocHeader* h) noexcept {
        switch ((AllocState)h->thisSize.state) {
            case AllocState::WILD_BLOCK:
                return 255;
            case AllocState::FREE: 
            {
                const U64 sz = h->thisSize.size;
                // Same mapping used everywhere: bin = ceil(sz/32) - 1, clamped to 254
                U64 bin = (U64)((sz - 1ull) >> 5);
                const U64 mask = (U64)-(bin > 254u);
                bin = (bin & ~mask) | (254u & mask);
                return bin;
            }
            case AllocState::INVALID:
            case AllocState::USED:
            case AllocState::BEGIN_SENTINEL:
            case AllocState::LARGE_BLOCK_SENTINEL:
            case AllocState::END_SENTINEL:
            default:
            {
               // Unreachable
               std::abort();
               return UINT_MAX;
            }
        }
    }
}}


// Public Allocator API Implementation
// ----------------------------------------------------------------------------------------------------------------
namespace ak {

    namespace priv {
        static constexpr Size HEADER_SIZE    = 16;
        static constexpr Size MIN_BLOCK_SIZE = 32;
        static constexpr Size ALIGNMENT      = 32;
    }
    
    inline const Char* to_string(AllocState s) noexcept {
        switch (s) {
            case AllocState::USED:                 return "USED";
            case AllocState::FREE:                 return "FREE";
            case AllocState::WILD_BLOCK:           return "WILD";
            case AllocState::BEGIN_SENTINEL:       return "SENTINEL B";
            case AllocState::LARGE_BLOCK_SENTINEL: return "SENTINEL L";
            case AllocState::END_SENTINEL:         return "SENTINEL E";
            default:                               return "INVALID";
        }
    }

    // In TryMalloc (replace existing function starting at 2933):
    /// \brief Attempts to synchronously allocate memory from the heap.
    /// 
    /// Algorithm:
    /// 1. Compute aligned block size: Add HEADER_SIZE and round up to ALIGNMENT.
    /// 2. Find smallest available bin >= required using SIMD-accelerated search.
    /// 3. For small bins (<254): Pop free block, split if larger than needed.
    /// 4. For medium bin (254): First-fit search on list, split if possible.
    /// 5. For wild bin (255): Split from wild block or allocate entirely if exact match.
    /// 
    /// Returns nullptr if no suitable block found (heap doesn't grow).
    /// For async version that suspends on failure, use co_await AllocMem(size).
    inline Void* try_alloc_mem(Size size) noexcept {
        using namespace priv;
        
        // Compute aligned block size
        Size maybeBlock = HEADER_SIZE + size;
        Size unaligned = maybeBlock & (ALIGNMENT - 1);
        Size requestedBlockSize = (unaligned != 0) ? maybeBlock + (ALIGNMENT - unaligned) : maybeBlock;
        assert((requestedBlockSize & (ALIGNMENT - 1)) == 0);
        assert(requestedBlockSize >= MIN_BLOCK_SIZE);
        
        // Find bin
        int binIdx = FindAllocFreeListBinIndex(&gKernel.alloc_table.freeListbinMask, requestedBlockSize);
        
        // Small bin allocation case
        // =========================
        if (binIdx < 254) {
            assert(gKernel.alloc_table.freeListBinsCount[binIdx] > 0);
            assert(GetAllocFreeBinBit(&gKernel.alloc_table.freeListbinMask, binIdx));
            
            utl::DLink* freeStack = &gKernel.alloc_table.freeListBins[binIdx];
            utl::DLink* link = utl::pop_link(freeStack);
            --gKernel.alloc_table.freeListBinsCount[binIdx];
            if (gKernel.alloc_table.freeListBinsCount[binIdx] == 0) {
                ClearAllocFreeBinBit(&gKernel.alloc_table.freeListbinMask, binIdx);
            } 
            
            AllocHeader* block = (AllocHeader*)((Char*)link - offsetof(FreeAllocHeader, freeListLink));
            AllocHeader* nextBlock = NextAllocHeaderPtr(block);
            __builtin_prefetch(nextBlock, 1, 3);
            
            if constexpr (IS_DEBUG_MODE) {
                utl::clear_link(link);
            }

            Size blockSize = block->thisSize.size;
            
            // Exact match case
            // ----------------
            if (blockSize == requestedBlockSize) {  
                // Update This State
                assert(block->thisSize.state == (U32)AllocState::FREE);
                block->thisSize.state = (U32)AllocState::USED;
                assert(block->thisSize.state == (U32)AllocState::USED);
                
                // Update Prev State
                assert(nextBlock->prevSize.state == (U32)AllocState::FREE);
                nextBlock->prevSize.state = (U32)AllocState::USED;
                assert(nextBlock->prevSize.state == (U32)AllocState::USED);

                gKernel.alloc_table.freeMemSize -= requestedBlockSize;
                ++gKernel.alloc_table.stats.alloc_cc[binIdx];
                ++gKernel.alloc_table.stats.reused_cc[binIdx];
                
                return (Void*)((Char*)block + HEADER_SIZE);
            } 
            
            // Required Split case
            // -------------------
            
            Size newFreeSize = blockSize - requestedBlockSize;
            assert(newFreeSize >= MIN_BLOCK_SIZE && newFreeSize % ALIGNMENT == 0);
            
            // Prefetch the new free block
            // ----------------------------
            FreeAllocHeader* newFree = (FreeAllocHeader*)((Char*)block + requestedBlockSize);
            __builtin_prefetch(newFree, 1, 3);

            // Prefetch stats
            // --------------
            Size newBinIdx = GetAllocSmallBinIndexFromSize(newFreeSize);
            __builtin_prefetch(&gKernel.alloc_table.stats.split_cc[binIdx], 1, 3);  
            __builtin_prefetch(&gKernel.alloc_table.stats.alloc_cc[binIdx], 1, 3);
            __builtin_prefetch(&gKernel.alloc_table.stats.pooled_cc[newBinIdx],  1, 3);

            // Update the new free block
            // -------------------------
            assert(block->thisSize.state == (U32)AllocState::FREE);

            AllocSizeRecord newAllocRecordSize = { requestedBlockSize, (U32)AllocState::USED, 0 };
            block->thisSize   = newAllocRecordSize;
            newFree->prevSize = newAllocRecordSize;

            AllocSizeRecord newFreeSizeRecord = { newFreeSize, (U32)AllocState::FREE, 0 };
            newFree->thisSize   = newFreeSizeRecord;
            nextBlock->prevSize = newFreeSizeRecord;
            
            assert(block->thisSize.state == (U32)AllocState::USED);
            assert(nextBlock->prevSize.state == (U32)AllocState::FREE);
            assert(newFree->thisSize.state == (U32)AllocState::FREE);

            // Update stats
            // ------------
            
            ++gKernel.alloc_table.stats.split_cc[binIdx];
            ++gKernel.alloc_table.stats.alloc_cc[binIdx];
            utl::push_link(&gKernel.alloc_table.freeListBins[newBinIdx], &newFree->freeListLink);
            SetAllocFreeBinBit(&gKernel.alloc_table.freeListbinMask, newBinIdx);
            ++gKernel.alloc_table.stats.pooled_cc[newBinIdx];            
            ++gKernel.alloc_table.freeListBinsCount[newBinIdx];
            gKernel.alloc_table.freeMemSize -= requestedBlockSize;
            
            return (Void*)((Char*)block + HEADER_SIZE);            
        }
        
        // Medium bin case do a fist fit search
        // ====================================
        if (binIdx == 254) {
            std::print("Medium bin case unimplemented\n");
            std::abort();
        }
        
        // Case we are allocating from the Wild Block (255)
        // ================================================
        if (binIdx == 255) {  
            assert(gKernel.alloc_table.wildBlock != nullptr);                      // Wild block pointer always valid
            assert(GetAllocFreeBinBit(&gKernel.alloc_table.freeListbinMask, 255)); // Wild block is always in the free list
            assert(gKernel.alloc_table.freeListBinsCount[255] == 1);               // Wild block has always exaclty one block

            // Note: The wild block is a degenerate case; it does not use free bins
            //       and it must always be allocated; which means have at least MIN_BLOCK_SIZE free space
            
            AllocHeader* oldWild = (AllocHeader*)gKernel.alloc_table.wildBlock;            
            
            // Prefetch the next block, the prev block and the new wild block
            // --------------------------------------------------------------
            
            // 1. Prefetch the next block
            AllocHeader* nextBlock = NextAllocHeaderPtr(oldWild);
            __builtin_prefetch(nextBlock, 1, 3);
            
            // 2. Prefetch the new wild block
            FreeAllocHeader* newWild = (FreeAllocHeader*)((Char*)oldWild + requestedBlockSize);
            __builtin_prefetch(newWild, 1, 3);

            // 3. Prefetch stats
            __builtin_prefetch(&gKernel.alloc_table.stats.alloc_cc[255], 1, 3);
            __builtin_prefetch(&gKernel.alloc_table.stats.split_cc[255], 1, 3);
            
            // Case there the wild block is full; memory is exhausted
            // ------------------------------------------------------
            Size oldSize = oldWild->thisSize.size;
            if (requestedBlockSize > oldSize - MIN_BLOCK_SIZE) {
                // the wild block must have at least MIN_BLOCK_SIZE free space
                ++gKernel.alloc_table.stats.failed_cc[255];
                return nullptr; // not enough space
            }
            
            // Case there is enough space -> Split the wild block
            // --------------------------------------------------
            Size newWildSize = oldSize - requestedBlockSize;
            assert(newWildSize >= MIN_BLOCK_SIZE && newWildSize % ALIGNMENT == 0);
            
            AllocSizeRecord allocatedSize = { requestedBlockSize, (U32)AllocState::USED, 0 };
            AllocHeader* allocated = oldWild;
            allocated->thisSize = allocatedSize;
            
            AllocSizeRecord newWildSizeRecord = { newWildSize, (U32)AllocState::WILD_BLOCK, 0 };
            newWild->thisSize = newWildSizeRecord;
            newWild->prevSize = allocated->thisSize;
            gKernel.alloc_table.wildBlock = newWild;
            nextBlock->prevSize = newWild->thisSize;
            
            // Update stats
            ++gKernel.alloc_table.stats.alloc_cc[255];
            ++gKernel.alloc_table.stats.split_cc[255];
            gKernel.alloc_table.freeMemSize -= requestedBlockSize;
            
            return (Void*)((Char*)allocated + HEADER_SIZE);
        }
        
        // Unreachable
        std::abort();
        return nullptr;
    }

    /// \brief Frees allocated memory and coalesces with adjacent free blocks.
    /// 
    /// Algorithm:
    /// 1. Locate the block from the pointer; return if null.
    /// 2. Perform left coalescing in a loop: while the previous block is free and leftMerges < sideCoalescing, unlink it from its bin, merge it into the current block by adjusting sizes and shifting the block pointer left, update merge stats.
    /// 3. Perform right coalescing in a loop: while the next block is free or wild and rightMerges < sideCoalescing, unlink it, merge into current by adjusting sizes, update next-next prevSize; if it was wild, flag mergedToWild and break the loop.
    /// 4. If mergedToWild, set the block state to WILD_BLOCK and update wild pointer; else, set to FREE and push to the appropriate bin.
    /// 5. Update global free memory size, free count stats, and final next block's prevSize.
    ///
    /// This handles chains of adjacent free blocks up to the limit per side.
    /// 
    /// \param ptr Pointer returned by TryMalloc (must not be nullptr).
    /// \param sideCoalescing Maximum number of merges per side (0 = no coalescing, defaults to UINT_MAX for unlimited).
    inline Void free_mem(Void* ptr, U32 sideCoalescing) noexcept {
        using namespace priv;
        assert(ptr != nullptr);
        (Void)sideCoalescing;

        // Release Block
        // -------------
        FreeAllocHeader* block = (FreeAllocHeader*)((Char*)ptr - HEADER_SIZE);
        AllocSizeRecord thisSize = block->thisSize;
        Size blockSize = thisSize.size;

        // Update block state
        // -------------------
        AllocState blockState = (AllocState)thisSize.state;
        assert(blockState == AllocState::USED);        
        block->thisSize.state = (U32)AllocState::FREE;

        // Update next block prevSize
        // --------------------------
        AllocHeader* nextBlock = NextAllocHeaderPtr((AllocHeader*)block);
        nextBlock->prevSize = block->thisSize;
        
        // Update stats
        // ------------
        unsigned origBinIdx = GetAllocSmallBinIndexFromSize(blockSize);
        assert(origBinIdx < 255);
        utl::push_link(&gKernel.alloc_table.freeListBins[origBinIdx], &block->freeListLink);
        ++gKernel.alloc_table.stats.free_cc[origBinIdx];
        ++gKernel.alloc_table.stats.pooled_cc[origBinIdx];
        ++gKernel.alloc_table.freeListBinsCount[origBinIdx];
        SetAllocFreeBinBit(&gKernel.alloc_table.freeListbinMask, origBinIdx);
        gKernel.alloc_table.freeMemSize += blockSize;
        
    }
}