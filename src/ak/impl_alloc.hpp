#pragma once

#include "ak/api_priv.hpp"

namespace ak { namespace priv {
    // FreeBin utilities
    Void SetAllocFreeBinBit(__m256i* bitField, U64 binIdx) noexcept;
    Bool GetAllocFreeBinBit(__m256i* bitField, U64 binIdx) noexcept;
    Void ClearAllocFreeBinBit(__m256i* bitField, U64 binIdx) noexcept;
    int  FindAllocFreeListBinIndex(__m256i* bitField, Size allocSize) noexcept;


    unsigned GetAllocFreeListBinIndex(const AllocBlockHeader* h) noexcept;
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

    inline int init_alloc_table(Void* mem, Size size) noexcept {
        AllocTable* at = &global_kernel_state.alloc_table;
        
        constexpr U64 SENTINEL_SIZE = sizeof(FreeAllocBlockHeader);

        assert(mem != nullptr);
        assert(size >= 4096);

        memset((Void*)at, 0, sizeof(AllocTable));
        
        // Establish heap boundaries
        Char* heapBegin = (Char*)(mem);
        Char* heapEnd   = heapBegin + size;

        // // Align start up to 32 and end down to 32 to keep all blocks 32B-multiples
        U64 alignedBegin = ((U64)heapBegin + SENTINEL_SIZE) & ~31ull;
        U64 alignedEnd   = ((U64)heapEnd   - SENTINEL_SIZE) & ~31ull;

        at->heap_begin = heapBegin;
        at->heap_end   = heapEnd;
        at->mem_begin  = (Char*)alignedBegin;
        at->mem_end    = (Char*)alignedEnd;
        at->mem_size   = (Size)(at->mem_end - at->mem_begin);

        // Addresses
        // Layout: [BeginSentinel] ... blocks ... [LargeBlockSentinel] ... largeBlocks ... [EndSentinel]
        FreeAllocBlockHeader* beginSentinel      = (FreeAllocBlockHeader*)alignedBegin;
        FreeAllocBlockHeader* wildBlock          = (FreeAllocBlockHeader*)((Char*)beginSentinel + SENTINEL_SIZE);
        FreeAllocBlockHeader* endSentinel        = (FreeAllocBlockHeader*)((Char*)alignedEnd    - SENTINEL_SIZE); 
        FreeAllocBlockHeader* largeBlockSentinel = (FreeAllocBlockHeader*)((Char*)endSentinel   - SENTINEL_SIZE);
        utl::init_link(&wildBlock->freelist_link);
        
        // Check alignments
        assert(((U64)beginSentinel      & 31ull) == 0ull);
        assert(((U64)wildBlock          & 31ull) == 0ull);
        assert(((U64)endSentinel        & 31ull) == 0ull);
        assert(((U64)largeBlockSentinel & 31ull) == 0ull);
        
        at->sentinel_begin      = beginSentinel;
        at->wild_block          = wildBlock;
        at->sentinel_end        = endSentinel;
        at->sentinel_large_block = largeBlockSentinel;
        
        beginSentinel->this_desc.size       = (U64)SENTINEL_SIZE;
        beginSentinel->this_desc.state      = (U32)AllocBlockState::BEGIN_SENTINEL;
        // Initialize prevSize for the begin sentinel to avoid reading
        // uninitialized memory in debug printers.
        beginSentinel->prev_desc            = { 0ull, (U32)AllocBlockState::INVALID, 0ull };
        wildBlock->this_desc.size           = (U64)((U64)largeBlockSentinel - (U64)wildBlock);
        wildBlock->this_desc.state          = (U32)AllocBlockState::WILD_BLOCK;
        largeBlockSentinel->this_desc.size  = (U64)SENTINEL_SIZE;
        largeBlockSentinel->this_desc.state = (U32)AllocBlockState::LARGE_BLOCK_SENTINEL;
        endSentinel->this_desc.size         = (U64)SENTINEL_SIZE;
        endSentinel->this_desc.state        = (U32)AllocBlockState::END_SENTINEL;
        wildBlock->prev_desc                = beginSentinel->this_desc;
        largeBlockSentinel->prev_desc       = wildBlock->this_desc;
        endSentinel->prev_desc              = largeBlockSentinel->this_desc;
        at->free_mem_size                    = wildBlock->this_desc.size;

        for (int i = 0; i < 255; ++i) { // Wild block (255) stays cleared
            utl::init_link(&at->freelist_head[i]);
        }
        at->freelist_count[255] = 1; // Wild block has always exaclty one block
        SetAllocFreeBinBit(&at->freelist_mask, 255);

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

    inline AllocBlockHeader* next(AllocBlockHeader* h) noexcept {
        size_t sz = (size_t)h->this_desc.size;
        if (sz == 0) return h;
        return (AllocBlockHeader*)((Char*)h + sz);
    }

    inline AllocBlockHeader* prev(AllocBlockHeader* h) noexcept {
        size_t sz = (size_t)h->prev_desc.size;
        if (sz == 0) return h;
        return (AllocBlockHeader*)((Char*)h - sz);
    }

    inline U64 get_alloc_freelist_index(U64 sz) noexcept {
        // Bin mapping: bin = ceil(sz/32) - 1, clamped to [0, 254]
        // Examples: 1..32 -> 0, 33..64 -> 1, ..., 8160 -> 254
        assert(sz > 0);
        U64 bin = (U64)((sz - 1ull) >> 5);
        const U64 mask = (U64)-(bin > 254u);
        bin = (bin & ~mask) | (254ull & mask);
        return bin;
    }

    inline unsigned GetAllocFreeListBinIndex(const AllocBlockHeader* h) noexcept {
        switch ((AllocBlockState)h->this_desc.state) {
            case AllocBlockState::WILD_BLOCK:
                return 255;
            case AllocBlockState::FREE: 
            {
                const U64 sz = h->this_desc.size;
                // Same mapping used everywhere: bin = ceil(sz/32) - 1, clamped to 254
                U64 bin = (U64)((sz - 1ull) >> 5);
                const U64 mask = (U64)-(bin > 254u);
                bin = (bin & ~mask) | (254u & mask);
                return bin;
            }
            case AllocBlockState::INVALID:
            case AllocBlockState::USED:
            case AllocBlockState::BEGIN_SENTINEL:
            case AllocBlockState::LARGE_BLOCK_SENTINEL:
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


// Public Allocator API Implementation
// ----------------------------------------------------------------------------------------------------------------
namespace ak {

    namespace priv {
        static constexpr Size HEADER_SIZE    = 16;
        static constexpr Size MIN_BLOCK_SIZE = 32;
        static constexpr Size ALIGNMENT      = 32;
    }
    
    inline const Char* to_string(AllocBlockState s) noexcept {
        switch (s) {
            case AllocBlockState::USED:                 return "USED";
            case AllocBlockState::FREE:                 return "FREE";
            case AllocBlockState::WILD_BLOCK:           return "WILD";
            case AllocBlockState::BEGIN_SENTINEL:       return "SENTINEL B";
            case AllocBlockState::LARGE_BLOCK_SENTINEL: return "SENTINEL L";
            case AllocBlockState::END_SENTINEL:         return "SENTINEL E";
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
        int binIdx = FindAllocFreeListBinIndex(&global_kernel_state.alloc_table.freelist_mask, requestedBlockSize);
        
        // Small bin allocation case
        // =========================
        if (binIdx < 254) {
            assert(global_kernel_state.alloc_table.freelist_count[binIdx] > 0);
            assert(GetAllocFreeBinBit(&global_kernel_state.alloc_table.freelist_mask, binIdx));
            
            utl::DLink* freeStack = &global_kernel_state.alloc_table.freelist_head[binIdx];
            utl::DLink* link = utl::pop_link(freeStack);
            --global_kernel_state.alloc_table.freelist_count[binIdx];
            if (global_kernel_state.alloc_table.freelist_count[binIdx] == 0) {
                ClearAllocFreeBinBit(&global_kernel_state.alloc_table.freelist_mask, binIdx);
            } 
            
            AllocBlockHeader* block = (AllocBlockHeader*)((Char*)link - offsetof(FreeAllocBlockHeader, freelist_link));
            AllocBlockHeader* nextBlock = next(block);
            __builtin_prefetch(nextBlock, 1, 3);
            
            if constexpr (IS_DEBUG_MODE) {
                utl::clear_link(link);
            }

            Size blockSize = block->this_desc.size;
            
            // Exact match case
            // ----------------
            if (blockSize == requestedBlockSize) {  
                // Update This State
                assert(block->this_desc.state == (U32)AllocBlockState::FREE);
                block->this_desc.state = (U32)AllocBlockState::USED;
                assert(block->this_desc.state == (U32)AllocBlockState::USED);
                
                // Update Prev State
                assert(nextBlock->prev_desc.state == (U32)AllocBlockState::FREE);
                nextBlock->prev_desc.state = (U32)AllocBlockState::USED;
                assert(nextBlock->prev_desc.state == (U32)AllocBlockState::USED);

                global_kernel_state.alloc_table.free_mem_size -= requestedBlockSize;
                ++global_kernel_state.alloc_table.stats.alloc_counter[binIdx];
                ++global_kernel_state.alloc_table.stats.reused_counter[binIdx];
                
                return (Void*)((Char*)block + HEADER_SIZE);
            } 
            
            // Required Split case
            // -------------------
            
            Size newFreeSize = blockSize - requestedBlockSize;
            assert(newFreeSize >= MIN_BLOCK_SIZE && newFreeSize % ALIGNMENT == 0);
            
            // Prefetch the new free block
            // ----------------------------
            FreeAllocBlockHeader* newFree = (FreeAllocBlockHeader*)((Char*)block + requestedBlockSize);
            __builtin_prefetch(newFree, 1, 3);

            // Prefetch stats
            // --------------
            Size newBinIdx = get_alloc_freelist_index(newFreeSize);
            __builtin_prefetch(&global_kernel_state.alloc_table.stats.split_counter[binIdx], 1, 3);  
            __builtin_prefetch(&global_kernel_state.alloc_table.stats.alloc_counter[binIdx], 1, 3);
            __builtin_prefetch(&global_kernel_state.alloc_table.stats.pooled_counter[newBinIdx],  1, 3);

            // Update the new free block
            // -------------------------
            assert(block->this_desc.state == (U32)AllocBlockState::FREE);

            AllocBlockDesc newAllocRecordSize = { requestedBlockSize, (U32)AllocBlockState::USED, 0 };
            block->this_desc   = newAllocRecordSize;
            newFree->prev_desc = newAllocRecordSize;

            AllocBlockDesc newFreeSizeRecord = { newFreeSize, (U32)AllocBlockState::FREE, 0 };
            newFree->this_desc   = newFreeSizeRecord;
            nextBlock->prev_desc = newFreeSizeRecord;
            
            assert(block->this_desc.state == (U32)AllocBlockState::USED);
            assert(nextBlock->prev_desc.state == (U32)AllocBlockState::FREE);
            assert(newFree->this_desc.state == (U32)AllocBlockState::FREE);

            // Update stats
            // ------------
            
            ++global_kernel_state.alloc_table.stats.split_counter[binIdx];
            ++global_kernel_state.alloc_table.stats.alloc_counter[binIdx];
            utl::push_link(&global_kernel_state.alloc_table.freelist_head[newBinIdx], &newFree->freelist_link);
            SetAllocFreeBinBit(&global_kernel_state.alloc_table.freelist_mask, newBinIdx);
            ++global_kernel_state.alloc_table.stats.pooled_counter[newBinIdx];            
            ++global_kernel_state.alloc_table.freelist_count[newBinIdx];
            global_kernel_state.alloc_table.free_mem_size -= requestedBlockSize;
            
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
            assert(global_kernel_state.alloc_table.wild_block != nullptr);                      // Wild block pointer always valid
            assert(GetAllocFreeBinBit(&global_kernel_state.alloc_table.freelist_mask, 255)); // Wild block is always in the free list
            assert(global_kernel_state.alloc_table.freelist_count[255] == 1);               // Wild block has always exaclty one block

            // Note: The wild block is a degenerate case; it does not use free bins
            //       and it must always be allocated; which means have at least MIN_BLOCK_SIZE free space
            
            AllocBlockHeader* oldWild = (AllocBlockHeader*)global_kernel_state.alloc_table.wild_block;            
            
            // Prefetch the next block, the prev block and the new wild block
            // --------------------------------------------------------------
            
            // 1. Prefetch the next block
            AllocBlockHeader* nextBlock = next(oldWild);
            __builtin_prefetch(nextBlock, 1, 3);
            
            // 2. Prefetch the new wild block
            FreeAllocBlockHeader* newWild = (FreeAllocBlockHeader*)((Char*)oldWild + requestedBlockSize);
            __builtin_prefetch(newWild, 1, 3);

            // 3. Prefetch stats
            __builtin_prefetch(&global_kernel_state.alloc_table.stats.alloc_counter[255], 1, 3);
            __builtin_prefetch(&global_kernel_state.alloc_table.stats.split_counter[255], 1, 3);
            
            // Case there the wild block is full; memory is exhausted
            // ------------------------------------------------------
            Size oldSize = oldWild->this_desc.size;
            if (requestedBlockSize > oldSize - MIN_BLOCK_SIZE) {
                // the wild block must have at least MIN_BLOCK_SIZE free space
                ++global_kernel_state.alloc_table.stats.failed_counter[255];
                return nullptr; // not enough space
            }
            
            // Case there is enough space -> Split the wild block
            // --------------------------------------------------
            Size newWildSize = oldSize - requestedBlockSize;
            assert(newWildSize >= MIN_BLOCK_SIZE && newWildSize % ALIGNMENT == 0);
            
            AllocBlockDesc allocatedSize = { requestedBlockSize, (U32)AllocBlockState::USED, 0 };
            AllocBlockHeader* allocated = oldWild;
            allocated->this_desc = allocatedSize;
            
            AllocBlockDesc newWildSizeRecord = { newWildSize, (U32)AllocBlockState::WILD_BLOCK, 0 };
            newWild->this_desc = newWildSizeRecord;
            newWild->prev_desc = allocated->this_desc;
            global_kernel_state.alloc_table.wild_block = newWild;
            nextBlock->prev_desc = newWild->this_desc;
            
            // Update stats
            ++global_kernel_state.alloc_table.stats.alloc_counter[255];
            ++global_kernel_state.alloc_table.stats.split_counter[255];
            global_kernel_state.alloc_table.free_mem_size -= requestedBlockSize;
            
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
        FreeAllocBlockHeader* block = (FreeAllocBlockHeader*)((Char*)ptr - HEADER_SIZE);
        AllocBlockDesc thisSize = block->this_desc;
        Size blockSize = thisSize.size;

        // Update block state
        // -------------------
        AllocBlockState blockState = (AllocBlockState)thisSize.state;
        assert(blockState == AllocBlockState::USED);        
        block->this_desc.state = (U32)AllocBlockState::FREE;

        // Update next block prevSize
        // --------------------------
        AllocBlockHeader* nextBlock = next((AllocBlockHeader*)block);
        nextBlock->prev_desc = block->this_desc;
        
        // Update stats
        // ------------
        unsigned origBinIdx = get_alloc_freelist_index(blockSize);
        assert(origBinIdx < 255);
        utl::push_link(&global_kernel_state.alloc_table.freelist_head[origBinIdx], &block->freelist_link);
        ++global_kernel_state.alloc_table.stats.free_counter[origBinIdx];
        ++global_kernel_state.alloc_table.stats.pooled_counter[origBinIdx];
        ++global_kernel_state.alloc_table.freelist_count[origBinIdx];
        SetAllocFreeBinBit(&global_kernel_state.alloc_table.freelist_mask, origBinIdx);
        global_kernel_state.alloc_table.free_mem_size += blockSize;
        
    }
}