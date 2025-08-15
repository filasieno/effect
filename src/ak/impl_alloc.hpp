#pragma once

#include "ak/api_priv.hpp"

namespace ak { namespace priv {
    // FreeBin utilities
    void SetAllocFreeBinBit(__m256i* bitField, U64 binIdx) noexcept;
    bool GetAllocFreeBinBit(__m256i* bitField, U64 binIdx) noexcept;
    void ClearAllocFreeBinBit(__m256i* bitField, U64 binIdx) noexcept;
    int  FindAllocFreeListBinIndex(__m256i* bitField, Size allocSize) noexcept;

    unsigned GetAllocSmallBinIndexFromSize(uint64_t sz) noexcept;
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

#if defined(__AVX2__)
        // AVX2 fast path (no runtime feature check)
        // Build a byte-granular mask: zero bytes < requiredByte, keep bytes >= requiredByte;
        // additionally mask bits < bitInByte in the requiredByte itself
        const unsigned requiredByte = requiredBin >> 3;   // 0..31
        const unsigned bitInByteReq = requiredBin & 7u;   // 0..7

        // Precomputed 0..31 index vector (one-time constant)
        alignas(32) static const unsigned char INDEX_0_31[32] = {
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
            16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
        };

        const __m256i availability = _mm256_load_si256((const __m256i*)bitField);
        const __m256i idx = _mm256_load_si256((const __m256i*)INDEX_0_31);
        const __m256i reqByteVec = _mm256_set1_epi8((char)requiredByte);
        const __m256i allOnes    = _mm256_set1_epi8((char)-1);

        // geMask: 0xFF where idx >= requiredByte, 0x00 otherwise (one compare + andnot)
        const __m256i ltMask = _mm256_cmpgt_epi8(reqByteVec, idx); // 0xFF where req > idx (i.e., idx < req)
        const __m256i geMask = _mm256_andnot_si256(ltMask, allOnes);

        // Apply byte-level mask and also clear bits below bitInByteReq in the required byte
        __m256i masked = _mm256_and_si256(availability, geMask);
        const __m256i eqMask = _mm256_cmpeq_epi8(idx, reqByteVec);
        const __m256i firstByteMaskVec = _mm256_set1_epi8((char)(unsigned char)(0xFFu << bitInByteReq));
        const __m256i onlyEqLane       = _mm256_and_si256(masked, eqMask);
        const __m256i maskedEqLane     = _mm256_and_si256(onlyEqLane, firstByteMaskVec);
        const __m256i otherLanes       = _mm256_andnot_si256(eqMask, masked);
        masked = _mm256_or_si256(otherLanes, maskedEqLane);

        // Find the lowest set bit using movemask on zero-compare
        const __m256i zero = _mm256_setzero_si256();
        const __m256i is_zero = _mm256_cmpeq_epi8(masked, zero);
        const unsigned non_zero_mask = ~static_cast<unsigned>(_mm256_movemask_epi8(is_zero));
        if (non_zero_mask == 0u) {
            return 255;
        }

        const int byte_idx = __builtin_ctz(non_zero_mask);
        // Read the target byte directly from the original bitfield and apply the per-byte bit mask for the first lane only (branchless)
        const unsigned char* bytes_src = (const unsigned char*)bitField;
        unsigned char byte_val = bytes_src[byte_idx];
        const unsigned char firstByteMaskScalar = (unsigned char)(0xFFu << bitInByteReq);
        const unsigned char sameMask = (unsigned char)-(byte_idx == (int)requiredByte); // 0xFF if same, 0x00 otherwise
        byte_val &= (unsigned char)((sameMask & firstByteMaskScalar) | (~sameMask));
        const int bit_in_byte = __builtin_ctz((unsigned)byte_val);
        return byte_idx * 8 + bit_in_byte;
#else
        // Fallback: branchless, fully unrolled scan of 4x64-bit words
        const uint64_t* words = (const uint64_t*)bitField; // 4 x 64-bit words
        const unsigned wordIdx = requiredBin >> 6;        // 0..3
        const unsigned bitInWord = requiredBin & 63u;    // 0..63

        const uint64_t startMask = ~0ull << bitInWord;   // valid when selecting the starting word

        // Enable masks for words >= word_idx (0xFFFFFFFFFFFFFFFF or 0x0)
        const uint64_t en0 = (uint64_t)-(0u >= wordIdx);
        const uint64_t en1 = (uint64_t)-(1u >= wordIdx);
        const uint64_t en2 = (uint64_t)-(2u >= wordIdx);
        const uint64_t en3 = (uint64_t)-(3u >= wordIdx);

        // Equal masks for exactly the starting word
        const uint64_t eq0 = (uint64_t)-(0u == wordIdx);
        const uint64_t eq1 = (uint64_t)-(1u == wordIdx);
        const uint64_t eq2 = (uint64_t)-(2u == wordIdx);
        const uint64_t eq3 = (uint64_t)-(3u == wordIdx);

        // Per-word effective masks: for starting word use startMask, otherwise ~0ull; then gate with enable mask
        const uint64_t mask0 = en0 & ((eq0 & startMask) | (~eq0 & ~0ull));
        const uint64_t mask1 = en1 & ((eq1 & startMask) | (~eq1 & ~0ull));
        const uint64_t mask2 = en2 & ((eq2 & startMask) | (~eq2 & ~0ull));
        const uint64_t mask3 = en3 & ((eq3 & startMask) | (~eq3 & ~0ull));

        const uint64_t v0 = words[0] & mask0;
        const uint64_t v1 = words[1] & mask1;
        const uint64_t v2 = words[2] & mask2;
        const uint64_t v3 = words[3] & mask3;

        const unsigned n0 = (unsigned)(v0 != 0);
        const unsigned n1 = (unsigned)(v1 != 0);
        const unsigned n2 = (unsigned)(v2 != 0);
        const unsigned n3 = (unsigned)(v3 != 0);
        const unsigned nonzero_groups = (n0) | (n1 << 1) | (n2 << 2) | (n3 << 3);
        if (nonzero_groups == 0u) {
            return 255;
        }

        const unsigned group = (unsigned)__builtin_ctz(nonzero_groups); // 0..3

        // Branchless selection of the first nonzero word
        const uint64_t sel0 = (uint64_t)-(group == 0u);
        const uint64_t sel1 = (uint64_t)-(group == 1u);
        const uint64_t sel2 = (uint64_t)-(group == 2u);
        const uint64_t sel3 = (uint64_t)-(group == 3u);
        const uint64_t vv = (v0 & sel0) | (v1 & sel1) | (v2 & sel2) | (v3 & sel3);

        const unsigned bit_in_word_first = (unsigned)__builtin_ctzll(vv);
        return (int)(group * 64u + bit_in_word_first);
#endif

    }

    inline int InitAllocTable(void* mem, Size size) noexcept {
        AllocTable* at = &gKernel.allocTable;
        
        constexpr U64 SENTINEL_SIZE = sizeof(FreeAllocHeader);

        assert(mem != nullptr);
        assert(size >= 4096);

        memset((void*)at, 0, sizeof(AllocTable));
        
        // Establish heap boundaries
        char* heapBegin = (char*)(mem);
        char* heapEnd   = heapBegin + size;

        // // Align start up to 32 and end down to 32 to keep all blocks 32B-multiples
        U64 alignedBegin = ((U64)heapBegin + SENTINEL_SIZE) & ~31ull;
        U64 alignedEnd   = ((U64)heapEnd   - SENTINEL_SIZE) & ~31ull;

        at->heapBegin = heapBegin;
        at->heapEnd   = heapEnd;
        at->memBegin  = (char*)alignedBegin;
        at->memEnd    = (char*)alignedEnd;
        at->memSize   = (Size)(at->memEnd - at->memBegin);

        // Addresses
        // Layout: [BeginSentinel] ... blocks ... [LargeBlockSentinel] ... largeBlocks ... [EndSentinel]
        FreeAllocHeader* beginSentinel      = (FreeAllocHeader*)alignedBegin;
        FreeAllocHeader* wildBlock          = (FreeAllocHeader*)((char*)beginSentinel + SENTINEL_SIZE);
        FreeAllocHeader* endSentinel        = (FreeAllocHeader*)((char*)alignedEnd - SENTINEL_SIZE); 
        FreeAllocHeader* largeBlockSentinel = (FreeAllocHeader*)((char*)endSentinel - SENTINEL_SIZE);
        InitLink(&wildBlock->freeListLink);
        
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
            InitLink(&at->freeListBins[i]);
        }
        at->freeListBinsCount[255] = 1; // Wild block has always exaclty one block
        SetAllocFreeBinBit(&at->freeListbinMask, 255);

        return 0;
    }

    inline void SetAllocFreeBinBit(__m256i* bitField, U64 binIdx) noexcept {
        assert(bitField != nullptr);
        assert(binIdx < 256);
        const U32 lane = (U32)(binIdx >> 6);         // 0..3
        const U32 bit  = (U32)(binIdx & 63u);        // 0..63
        U64* lanes = reinterpret_cast<U64*>(bitField); // 4 x 64-bit lanes in 256-bit vector
        lanes[lane] |= (1ull << bit);
    }

    inline bool GetAllocFreeBinBit(__m256i* bitField, U64 binIdx) noexcept {
        assert(bitField != nullptr);
        assert(binIdx < 256);
        const U64 lane = binIdx >> 6;         // 0..3
        const U64 bit  = binIdx & 63u;        // 0..63
        const U64* lanes = reinterpret_cast<const U64*>(bitField);
        return ((lanes[lane] >> bit) & 1ull) != 0ull;
    }

    inline void ClearAllocFreeBinBit(__m256i* bitField, U64 binIdx) noexcept {
        assert(bitField != nullptr);
        assert(binIdx < 256);
        const U64 lane = binIdx >> 6;         // 0..3
        const U64 bit  = binIdx & 63u;        // 0..63
        U64* lanes = reinterpret_cast<U64*>(bitField);
        lanes[lane] &= ~(1ull << bit);
    }

    inline AllocHeader* NextAllocHeaderPtr(AllocHeader* h) noexcept {
        size_t sz = (size_t)h->thisSize.size;
        if (sz == 0) return h;
        return (AllocHeader*)((char*)h + sz);
    }

    inline AllocHeader* PrevAllocHeaderPtr(AllocHeader* h) noexcept {
        size_t sz = (size_t)h->prevSize.size;
        if (sz == 0) return h;
        return (AllocHeader*)((char*)h - sz);
    }

    inline unsigned GetAllocSmallBinIndexFromSize(uint64_t sz) noexcept {
        // Bin mapping: bin = ceil(sz/32) - 1, clamped to [0, 254]
        // Examples: 1..32 -> 0, 33..64 -> 1, ..., 8160 -> 254
        assert(sz > 0);
        unsigned bin = (unsigned)((sz - 1ull) >> 5); // ceil(sz/32) - 1
        if (bin > 254u) bin = 254u;                  // 254 = medium, 255 = wild
        return bin;
    }

    inline unsigned GetAllocFreeListBinIndex(const AllocHeader* h) noexcept {
        switch ((AllocState)h->thisSize.state) {
            case AllocState::WILD_BLOCK:
                return 255;
            case AllocState::FREE: 
            {
                const Size sz = h->thisSize.size;
                // Same mapping used everywhere: bin = ceil(sz/32) - 1, clamped to 254
                unsigned bin = (unsigned)((sz - 1ull) >> 5);
                if (bin > 254u) bin = 254u;
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
    
    inline const char* ToString(AllocState s) noexcept {
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
    inline void* TryMalloc(Size size) noexcept {
        using namespace priv;
        AllocTable* at = &gKernel.allocTable;
        
        // Compute aligned block size
        Size maybeBlock = HEADER_SIZE + size;
        Size unaligned = maybeBlock & (ALIGNMENT - 1);
        Size requestedBlockSize = (unaligned != 0) ? maybeBlock + (ALIGNMENT - unaligned) : maybeBlock;
        assert((requestedBlockSize & (ALIGNMENT - 1)) == 0);
        assert(requestedBlockSize >= MIN_BLOCK_SIZE);
        
        // Find bin
        int binIdx = FindAllocFreeListBinIndex(&at->freeListbinMask, requestedBlockSize);
        // assert(GetAllocFreeBinBit(&at->freeListbinMask, binIdx));
        // assert(!IsLinkDetached(&at->freeListBins[binIdx]));
        // assert(at->freeListBinsCount[binIdx] > 0);
        
        // Small bin allocation case
        // =========================
        if (binIdx < 254) {
            DLink* freeStack = &at->freeListBins[binIdx];
            DLink* link = PopLink(freeStack);
            --at->freeListBinsCount[binIdx];
            if (at->freeListBinsCount[binIdx] == 0) ClearAllocFreeBinBit(&at->freeListbinMask, binIdx);
            
            AllocHeader* block = (AllocHeader*)((char*)link - offsetof(FreeAllocHeader, freeListLink));
            AllocHeader* nextBlock = NextAllocHeaderPtr(block);
            __builtin_prefetch(nextBlock, 1, 3);
            
            if constexpr (IS_DEBUG_MODE) {
                ClearLink(link);
            }

            Size blockSize = block->thisSize.size;
            
            // Exact match case
            // ----------------
            if (blockSize == requestedBlockSize) {  
                block->thisSize.state = (U32)AllocState::USED;
                at->freeMemSize -= requestedBlockSize;
                AllocStats* stats = &at->stats;
                ++stats->binAllocCount[binIdx];
                ++stats->binReuseCount[binIdx];
                nextBlock->prevSize.state = (U32)AllocState::USED;
                return (void*)((char*)block + HEADER_SIZE);
            } 

            // Required Split case
            // -------------------
            Size newFreeSize = blockSize - requestedBlockSize;
            assert(newFreeSize >= MIN_BLOCK_SIZE && newFreeSize % ALIGNMENT == 0);
            
            FreeAllocHeader* newFree = (FreeAllocHeader*)((char*)block + requestedBlockSize);
            __builtin_prefetch(newFree, 1, 3);
            
            block->thisSize.size = requestedBlockSize;
            block->thisSize.state = (U32)AllocState::USED;
            
            newFree->thisSize.size = newFreeSize;
            newFree->thisSize.state = (U32)AllocState::FREE;
            newFree->prevSize = block->thisSize;
            
            nextBlock->prevSize = newFree->thisSize;
            
            int newBinIdx = (newFreeSize / ALIGNMENT) - 1;
            DLink* newStack = &at->freeListBins[newBinIdx];
            PushLink(newStack, &newFree->freeListLink);
            ++at->freeListBinsCount[newBinIdx];
            if (at->freeListBinsCount[newBinIdx] == 1) SetAllocFreeBinBit(&at->freeListbinMask, newBinIdx);
            
            AllocStats* stats = &at->stats;
            ++stats->binAllocCount[binIdx];
            ++stats->binSplitCount[binIdx];
            ++stats->binPoolCount[newBinIdx];
            at->freeMemSize -= requestedBlockSize;
            
            return (void*)((char*)block + HEADER_SIZE);
            
        }
        
        // Medium bin case do a fist fit search
        // ====================================
        if (binIdx == 254) {
            DLink* mediumList = &at->freeListBins[254];
            for (DLink* link = mediumList->next; link != mediumList; link = link->next) {
                FreeAllocHeader* block = (FreeAllocHeader*)((char*)link - offsetof(FreeAllocHeader, freeListLink));
                Size blockSize = block->thisSize.size;
                if (blockSize >= requestedBlockSize) {
                    DetachLink(link);
                    --at->freeListBinsCount[254];
                    if (at->freeListBinsCount[254] == 0) ClearAllocFreeBinBit(&at->freeListbinMask, 254);
                    
                    AllocHeader* nextBlock = NextAllocHeaderPtr((AllocHeader*)block);
                    __builtin_prefetch(nextBlock, 1, 3);
                    
                    if constexpr (IS_DEBUG_MODE) {
                        ClearLink(link);
                    }         

                    // Exact match case
                    // ----------------
                    if (blockSize == requestedBlockSize) {  
                        block->thisSize.state = (U32)AllocState::USED;
                        at->freeMemSize -= requestedBlockSize;
                        AllocStats* stats = &at->stats;
                        ++stats->binAllocCount[254];
                        ++stats->binReuseCount[254];
                        nextBlock->prevSize.state = (U32)AllocState::USED;
                        return (void*)((char*)block + HEADER_SIZE);
                    } 

                    // Required Split case
                    // -------------------
                    Size newFreeSize = blockSize - requestedBlockSize;
                    assert(newFreeSize >= MIN_BLOCK_SIZE && newFreeSize % ALIGNMENT == 0);
                    
                    FreeAllocHeader* newFree = (FreeAllocHeader*)((char*)block + requestedBlockSize);
                    __builtin_prefetch(newFree, 1, 3);
                    
                    block->thisSize.size = requestedBlockSize;
                    block->thisSize.state = (U32)AllocState::USED;
                    
                    newFree->thisSize.size = newFreeSize;
                    newFree->thisSize.state = (U32)AllocState::FREE;
                    newFree->prevSize = block->thisSize;
                    
                    nextBlock->prevSize = newFree->thisSize;
                    
                    // For medium remainder, push back to medium bin (254)
                    U64 newBinIdx = GetAllocSmallBinIndexFromSize((U64)newFreeSize);
                    DLink* newStack = &at->freeListBins[newBinIdx];
                    PushLink(newStack, &newFree->freeListLink);
                    ++at->freeListBinsCount[newBinIdx];
                    SetAllocFreeBinBit(&at->freeListbinMask, newBinIdx);
                    
                    AllocStats* stats = &at->stats;
                    ++stats->binAllocCount[254];
                    ++stats->binSplitCount[254];
                    ++stats->binPoolCount[newBinIdx];
                    at->freeMemSize -= requestedBlockSize;
                    
                    return (void*)((char*)block + HEADER_SIZE);
                
                }
            }
            return nullptr;  // No fit found
        }
        
        // Case we are allocating from the Wild Block (255)
        // ================================================
        if (binIdx == 255) {  
            assert(at->wildBlock != nullptr);                      // Wild block pointer always valid
            assert(GetAllocFreeBinBit(&at->freeListbinMask, 255)); // Wild block is always in the free list
            assert(at->freeListBinsCount[255] == 1);               // Wild block has always exaclty one block

            // Note: The wild block is a degenerate case; it does not use free bins
            //       and it must always be allocated; which means have at least MIN_BLOCK_SIZE free space
            
            AllocHeader* oldWild = (AllocHeader*)at->wildBlock;            
            
            // Prefetch the next block, the prev block and the new wild block
            // --------------------------------------------------------------
            
            // 1. Prefetch the next block
            AllocHeader* nextBlock = NextAllocHeaderPtr(oldWild);
            __builtin_prefetch(nextBlock, 1, 3);
            
            // 2. Prefetch the new wild block
            FreeAllocHeader* newWild = (FreeAllocHeader*)((char*)oldWild + requestedBlockSize);
            __builtin_prefetch(newWild, 1, 3);

            // 3. Prefetch stats
            __builtin_prefetch(&at->stats.binAllocCount[255], 1, 3);
            __builtin_prefetch(&at->stats.binSplitCount[255], 1, 3);
            
            // Case there the wild block is full; memory is exhausted
            // ------------------------------------------------------
            Size oldSize = oldWild->thisSize.size;
            if (requestedBlockSize > oldSize - MIN_BLOCK_SIZE) {
                // the wild block must have at least MIN_BLOCK_SIZE free space
                ++at->stats.binFailedCount[255];
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
            at->wildBlock = newWild;
            nextBlock->prevSize = newWild->thisSize;
            
            // Update stats
            ++at->stats.binAllocCount[255];
            ++at->stats.binSplitCount[255];
            at->freeMemSize -= requestedBlockSize;
            
            return (void*)((char*)allocated + HEADER_SIZE);
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
    inline void FreeMem(void* ptr, unsigned sideCoalescing = UINT_MAX) noexcept {
        using namespace priv;
        AllocTable* at = &gKernel.allocTable;

        if (ptr == nullptr) return;

        FreeAllocHeader* block = (FreeAllocHeader*)((char*)ptr - HEADER_SIZE);
        assert(block->thisSize.state == (U32)AllocState::USED);

        AllocHeader* nextBlock = NextAllocHeaderPtr((AllocHeader*)block);
        Size blockSize = block->thisSize.size;
        unsigned origBinIdx = GetAllocFreeListBinIndex((AllocHeader*)block);  // For stats

        // Step 2: Left coalescing loop - merge previous free blocks backwards
        unsigned leftMerges = 0;
        while (leftMerges < sideCoalescing) {
            AllocHeader* prevBlock = PrevAllocHeaderPtr((AllocHeader*)block);
            if (prevBlock->thisSize.state != (U32)AllocState::FREE) break;  // Stop if not free

            FreeAllocHeader* prevFree = (FreeAllocHeader*)prevBlock;
            int prevBin = GetAllocFreeListBinIndex((AllocHeader*)prevFree);

            // Unlink the previous free block from its bin
            DetachLink(&prevFree->freeListLink);
            --at->freeListBinsCount[prevBin];
            if (at->freeListBinsCount[prevBin] == 0) ClearAllocFreeBinBit(&at->freeListbinMask, prevBin);

            // Merge: extend prev into current by adding sizes, shift block to prev
            prevBlock->thisSize.size += blockSize;
            block = (FreeAllocHeader*)prevBlock;
            blockSize = block->thisSize.size;

            // Update stats for this merge
            AllocStats* stats = &at->stats;
            ++stats->binMergeCount[prevBin];

            ++leftMerges;
        }

        // Step 3: Right coalescing loop - merge next free/wild blocks forwards
        unsigned rightMerges = 0;
        bool mergedToWild = false;
        while (rightMerges < sideCoalescing) {
            nextBlock = NextAllocHeaderPtr((AllocHeader*)block);  // Refresh next after any prior merges
            AllocState nextState = (AllocState)nextBlock->thisSize.state;
            if (nextState != AllocState::FREE && nextState != AllocState::WILD_BLOCK) break;  // Stop if not free/wild

            FreeAllocHeader* nextFree = (FreeAllocHeader*)nextBlock;
            Size nextSize = nextBlock->thisSize.size;

            if (nextState == AllocState::FREE) {
                int nextBin = GetAllocFreeListBinIndex((AllocHeader*)nextFree);

                // Unlink the next free block from its bin
                DetachLink(&nextFree->freeListLink);
                --at->freeListBinsCount[nextBin];
                if (at->freeListBinsCount[nextBin] == 0) ClearAllocFreeBinBit(&at->freeListbinMask, nextBin);

                // Update stats for this merge
                AllocStats* stats = &at->stats;
                ++stats->binMergeCount[nextBin];
            } else {  // Wild block
                assert(nextFree == at->wildBlock);

                // Unlink the wild block
                DetachLink(&nextFree->freeListLink);
                at->freeListBinsCount[255] = 0;
                ClearAllocFreeBinBit(&at->freeListbinMask, 255);

                // Update stats and flag
                AllocStats* stats = &at->stats;
                ++stats->binMergeCount[255];
                mergedToWild = true;
            }

            // Merge: extend current into next by adding sizes
            block->thisSize.size += nextSize;
            blockSize = block->thisSize.size;

            // Update the block after next's prevSize to point to the expanded current
            AllocHeader* nextNext = NextAllocHeaderPtr(nextBlock);
            nextNext->prevSize = block->thisSize;

            ++rightMerges;

            // If we merged the wild block, stop further right coalescing
            if (mergedToWild) break;
        }

        // Step 4: Set final state based on whether we merged to wild
        if (mergedToWild) {
            block->thisSize.state = (U32)AllocState::WILD_BLOCK;
            at->wildBlock = block;
            // Wild doesn't get pushed to a bin
        } else {
            block->thisSize.state = (U32)AllocState::FREE;
            int newBinIdx = (blockSize / ALIGNMENT) - 1;
            DLink* newStack = &at->freeListBins[newBinIdx];
            PushLink(newStack, &block->freeListLink);
            ++at->freeListBinsCount[newBinIdx];
            if (at->freeListBinsCount[newBinIdx] == 1) SetAllocFreeBinBit(&at->freeListbinMask, newBinIdx);

            // Update pool stats for the new free block
            AllocStats* stats = &at->stats;
            ++stats->binPoolCount[newBinIdx];
        }

        // Step 5: Update global stats and final metadata
        at->freeMemSize += blockSize;  // Add the total merged size to free memory
        AllocStats* stats = &at->stats;
        ++stats->binFreeCount[origBinIdx];

        // Ensure the final next block's prevSize is updated
        nextBlock = NextAllocHeaderPtr((AllocHeader*)block);
        nextBlock->prevSize = block->thisSize;
    }
}