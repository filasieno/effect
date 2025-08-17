#pragma once

#include "ak/api_priv.hpp"

namespace ak { namespace priv {
    // FreeBin utilities
    Void set_alloc_freelist_mask(__m256i* bit_field, U64 bin_idx) noexcept;
    Bool get_alloc_freelist_mask(__m256i* bit_field, U64 bin_idx) noexcept;
    Void clear_alloc_freelist_mask(__m256i* bit_field, U64 bin_idx) noexcept;
    int  find_alloc_freelist_index(__m256i* bit_field, Size alloc_size) noexcept;


    unsigned get_alloc_freelist_index(const AllocBlockHeader* header) noexcept;
}}



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
    inline int find_alloc_freelist_index(__m256i* bit_field, Size alloc_size) noexcept {
        assert(bit_field != nullptr);
        assert(((uintptr_t)bit_field % 64ull) == 0ull);

        // Compute the starting bin index (ceil(alloc_size/32) - 1), clamped to [0,255]
        unsigned required_bin = 0u;
        if (alloc_size != 0) {
            required_bin = (unsigned)((alloc_size - 1u) >> 5); // floor((allocSize-1)/32)
        }
        if (required_bin > 255u) required_bin = 255u;

        // Safely load as 4x64-bit words to avoid strict-aliasing pitfalls
        uint64_t words[4];
        std::memcpy(words, bit_field, sizeof(words));

        const unsigned starting_word_index = required_bin >> 6; // 0..3
        const unsigned bit_in_word         = required_bin & 63u; // 0..63

        for (unsigned word_index = starting_word_index; word_index < 4u; ++word_index) {
            const uint64_t mask = (word_index == starting_word_index) ? (~0ull << bit_in_word) : ~0ull;
            const uint64_t value = words[word_index] & mask;
            if (value != 0ull) {
                return (int)(word_index * 64u + (unsigned)__builtin_ctzll(value));
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
        Char* heap_begin = (Char*)(mem);
        Char* heap_end   = heap_begin + size;

        // // Align start up to 32 and end down to 32 to keep all blocks 32B-multiples
        U64 aligned_begin = ((U64)heap_begin + SENTINEL_SIZE) & ~31ull;
        U64 aligned_end   = ((U64)heap_end   - SENTINEL_SIZE) & ~31ull;

        at->heap_begin = heap_begin;
        at->heap_end   = heap_end;
        at->mem_begin  = (Char*)aligned_begin;
        at->mem_end    = (Char*)aligned_end;
        at->mem_size   = (Size)(at->mem_end - at->mem_begin);

        // Addresses
        // Layout: [BeginSentinel] ... blocks ... [LargeBlockSentinel] ... largeBlocks ... [EndSentinel]
        FreeAllocBlockHeader* begin_sentinel      = (FreeAllocBlockHeader*)aligned_begin;
        FreeAllocBlockHeader* wild_block          = (FreeAllocBlockHeader*)((Char*)begin_sentinel + SENTINEL_SIZE);
        FreeAllocBlockHeader* end_sentinel        = (FreeAllocBlockHeader*)((Char*)aligned_end    - SENTINEL_SIZE); 
        FreeAllocBlockHeader* large_block_sentinel = (FreeAllocBlockHeader*)((Char*)end_sentinel   - SENTINEL_SIZE);
        utl::init_link(&wild_block->freelist_link);
        
        // Check alignments
        assert(((U64)begin_sentinel       & 31ull) == 0ull);
        assert(((U64)wild_block           & 31ull) == 0ull);
        assert(((U64)end_sentinel         & 31ull) == 0ull);
        assert(((U64)large_block_sentinel & 31ull) == 0ull);
        
        at->sentinel_begin       = begin_sentinel;
        at->wild_block           = wild_block;
        at->sentinel_end         = end_sentinel;
        at->sentinel_large_block = large_block_sentinel;
        
        begin_sentinel->this_desc.size       = (U64)SENTINEL_SIZE;
        begin_sentinel->this_desc.state      = (U32)AllocBlockState::BEGIN_SENTINEL;
        // Initialize prevSize for the begin sentinel to avoid reading
        // uninitialized memory in debug printers.
        begin_sentinel->prev_desc             = { 0ull, (U32)AllocBlockState::INVALID, 0ull };
        wild_block->this_desc.size            = (U64)((U64)large_block_sentinel - (U64)wild_block);
        wild_block->this_desc.state           = (U32)AllocBlockState::WILD_BLOCK;
        large_block_sentinel->this_desc.size  = (U64)SENTINEL_SIZE;
        large_block_sentinel->this_desc.state = (U32)AllocBlockState::LARGE_BLOCK_SENTINEL;
        end_sentinel->this_desc.size          = (U64)SENTINEL_SIZE;
        end_sentinel->this_desc.state         = (U32)AllocBlockState::END_SENTINEL;
        wild_block->prev_desc                 = begin_sentinel->this_desc;
        large_block_sentinel->prev_desc       = wild_block->this_desc;
        end_sentinel->prev_desc               = large_block_sentinel->this_desc;
        at->free_mem_size                     = wild_block->this_desc.size;

        for (int i = 0; i < 255; ++i) { // Wild block (255) stays cleared
            utl::init_link(&at->freelist_head[i]);
        }
        at->freelist_count[255] = 1; // Wild block has always exaclty one block
        set_alloc_freelist_mask(&at->freelist_mask, 255);

        return 0;
    }

    inline Void set_alloc_freelist_mask(__m256i* bit_field, U64 bin_idx) noexcept {
        assert(bit_field != nullptr);
        assert(bin_idx < 256);
        const U64 lane = bin_idx >> 6;  // 0..3
        const U64 bit  = bin_idx & 63u; // 0..63
        uint64_t word;
        std::memcpy(&word, reinterpret_cast<const Char*>(bit_field) + lane * sizeof(uint64_t), sizeof(uint64_t));
        word |= (1ull << bit);
        std::memcpy(reinterpret_cast<Char*>(bit_field) + lane * sizeof(uint64_t), &word, sizeof(uint64_t));
    }

    inline Bool get_alloc_freelist_mask(__m256i* bit_field, U64 bin_idx) noexcept {
        assert(bit_field != nullptr);
        assert(bin_idx < 256);
        const U64 lane = bin_idx >> 6;  // 0..3
        const U64 bit  = bin_idx & 63u; // 0..63
        uint64_t word;
        std::memcpy(&word, reinterpret_cast<const Char*>(bit_field) + lane * sizeof(uint64_t), sizeof(uint64_t));
        return ((word >> bit) & 1ull) != 0ull;
    }

    inline Void clear_alloc_freelist_mask(__m256i* bit_field, U64 bin_idx) noexcept {
        assert(bit_field != nullptr);
        assert(bin_idx < 256);
        const U64 lane = bin_idx >> 6;  // 0..3
        const U64 bit  = bin_idx & 63u; // 0..63
        uint64_t word;
        std::memcpy(&word, reinterpret_cast<const Char*>(bit_field) + lane * sizeof(uint64_t), sizeof(uint64_t));
        word &= ~(1ull << bit);
        std::memcpy(reinterpret_cast<Char*>(bit_field) + lane * sizeof(uint64_t), &word, sizeof(uint64_t));
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
        // Bin mapping: bin = ceil(sz/32) - 1, clamped to [0, 254]
        // Examples: 1..32 -> 0, 33..64 -> 1, ..., 8160 -> 254
        assert(sz > 0);
        U64 bin = (U64)((sz - 1ull) >> 5);
        const U64 mask = (U64)-(bin > 254u);
        bin = (bin & ~mask) | (254ull & mask);
        return bin;
    }

    inline unsigned get_alloc_freelist_index(const AllocBlockHeader* header) noexcept {
        switch ((AllocBlockState)header->this_desc.state) {
            case AllocBlockState::WILD_BLOCK:
                return 255;
            case AllocBlockState::FREE: 
            {
                const U64 sz = header->this_desc.size;
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
        Size maybe_block = HEADER_SIZE + size;
        Size unaligned = maybe_block & (ALIGNMENT - 1);
        Size requested_block_size = (unaligned != 0) ? maybe_block + (ALIGNMENT - unaligned) : maybe_block;
        assert((requested_block_size & (ALIGNMENT - 1)) == 0);
        assert(requested_block_size >= MIN_BLOCK_SIZE);
        
        // Find bin
        int bin_idx = find_alloc_freelist_index(&global_kernel_state.alloc_table.freelist_mask, requested_block_size);
        
        // Small bin allocation case
        // =========================
        if (bin_idx < 254) {
            assert(global_kernel_state.alloc_table.freelist_count[bin_idx] > 0);
            assert(get_alloc_freelist_mask(&global_kernel_state.alloc_table.freelist_mask, bin_idx));
            
            utl::DLink* free_stack = &global_kernel_state.alloc_table.freelist_head[bin_idx];
            utl::DLink* link = utl::pop_link(free_stack);
            --global_kernel_state.alloc_table.freelist_count[bin_idx];
            if (global_kernel_state.alloc_table.freelist_count[bin_idx] == 0) {
                clear_alloc_freelist_mask(&global_kernel_state.alloc_table.freelist_mask, bin_idx);
            } 
            
            AllocBlockHeader* block = (AllocBlockHeader*)((Char*)link - offsetof(FreeAllocBlockHeader, freelist_link));
            AllocBlockHeader* next_block = next(block);
            __builtin_prefetch(next_block, 1, 3);
            
            if constexpr (IS_DEBUG_MODE) {
                utl::clear_link(link);
            }

            Size block_size = block->this_desc.size;
            
            // Exact match case
            // ----------------
            if (block_size == requested_block_size) {  
                // Update This State
                assert(block->this_desc.state == (U32)AllocBlockState::FREE);
                block->this_desc.state = (U32)AllocBlockState::USED;
                assert(block->this_desc.state == (U32)AllocBlockState::USED);
                
                // Update Prev State
                assert(next_block->prev_desc.state == (U32)AllocBlockState::FREE);
                next_block->prev_desc.state = (U32)AllocBlockState::USED;
                assert(next_block->prev_desc.state == (U32)AllocBlockState::USED);

                global_kernel_state.alloc_table.free_mem_size -= requested_block_size;
                ++global_kernel_state.alloc_table.stats.alloc_counter[bin_idx];
                ++global_kernel_state.alloc_table.stats.reused_counter[bin_idx];
                
                return (Void*)((Char*)block + HEADER_SIZE);
            } 
            
            // Required Split case
            // -------------------
            
            Size new_free_size = block_size - requested_block_size;
            assert(new_free_size >= MIN_BLOCK_SIZE && new_free_size % ALIGNMENT == 0);
            
            // Prefetch the new free block
            // ----------------------------
            FreeAllocBlockHeader* new_free = (FreeAllocBlockHeader*)((Char*)block + requested_block_size);
            __builtin_prefetch(new_free, 1, 3);

            // Prefetch stats
            // --------------
            Size new_bin_idx = get_alloc_freelist_index(new_free_size);
            __builtin_prefetch(&global_kernel_state.alloc_table.stats.split_counter[bin_idx], 1, 3);  
            __builtin_prefetch(&global_kernel_state.alloc_table.stats.alloc_counter[bin_idx], 1, 3);
            __builtin_prefetch(&global_kernel_state.alloc_table.stats.pooled_counter[new_bin_idx],  1, 3);

            // Update the new free block
            // -------------------------
            assert(block->this_desc.state == (U32)AllocBlockState::FREE);

            AllocBlockDesc new_alloc_record_size = { requested_block_size, (U32)AllocBlockState::USED, 0 };
            block->this_desc   = new_alloc_record_size;
            new_free->prev_desc = new_alloc_record_size;

            AllocBlockDesc new_free_size_record = { new_free_size, (U32)AllocBlockState::FREE, 0 };
            new_free->this_desc   = new_free_size_record;
            next_block->prev_desc = new_free_size_record;
            
            assert(block->this_desc.state == (U32)AllocBlockState::USED);
            assert(next_block->prev_desc.state == (U32)AllocBlockState::FREE);
            assert(new_free->this_desc.state == (U32)AllocBlockState::FREE);

            // Update stats
            // ------------
            
            ++global_kernel_state.alloc_table.stats.split_counter[bin_idx];
            ++global_kernel_state.alloc_table.stats.alloc_counter[bin_idx];
            utl::push_link(&global_kernel_state.alloc_table.freelist_head[new_bin_idx], &new_free->freelist_link);
            set_alloc_freelist_mask(&global_kernel_state.alloc_table.freelist_mask, new_bin_idx);
            ++global_kernel_state.alloc_table.stats.pooled_counter[new_bin_idx];            
            ++global_kernel_state.alloc_table.freelist_count[new_bin_idx];
            global_kernel_state.alloc_table.free_mem_size -= requested_block_size;
            
            return (Void*)((Char*)block + HEADER_SIZE);            
        }
        
        // Medium bin case do a fist fit search
        // ====================================
        if (bin_idx == 254) {
            std::print("Medium bin case unimplemented\n");
            std::abort();
        }
        
        // Case we are allocating from the Wild Block (255)
        // ================================================
        if (bin_idx == 255) {  
            assert(global_kernel_state.alloc_table.wild_block != nullptr);                      // Wild block pointer always valid
            assert(get_alloc_freelist_mask(&global_kernel_state.alloc_table.freelist_mask, 255)); // Wild block is always in the free list
            assert(global_kernel_state.alloc_table.freelist_count[255] == 1);               // Wild block has always exaclty one block

            // Note: The wild block is a degenerate case; it does not use free bins
            //       and it must always be allocated; which means have at least MIN_BLOCK_SIZE free space
            
            AllocBlockHeader* old_wild = (AllocBlockHeader*)global_kernel_state.alloc_table.wild_block;            
            
            // Prefetch the next block, the prev block and the new wild block
            // --------------------------------------------------------------
            
            // 1. Prefetch the next block
            AllocBlockHeader* next_block = next(old_wild);
            __builtin_prefetch(next_block, 1, 3);
            
            // 2. Prefetch the new wild block
            FreeAllocBlockHeader* new_wild = (FreeAllocBlockHeader*)((Char*)old_wild + requested_block_size);
            __builtin_prefetch(new_wild, 1, 3);

            // 3. Prefetch stats
            __builtin_prefetch(&global_kernel_state.alloc_table.stats.alloc_counter[255], 1, 3);
            __builtin_prefetch(&global_kernel_state.alloc_table.stats.split_counter[255], 1, 3);
            
            // Case there the wild block is full; memory is exhausted
            // ------------------------------------------------------
            Size old_size = old_wild->this_desc.size;
            if (requested_block_size > old_size - MIN_BLOCK_SIZE) {
                // the wild block must have at least MIN_BLOCK_SIZE free space
                ++global_kernel_state.alloc_table.stats.failed_counter[255];
                return nullptr; // not enough space
            }
            
            // Case there is enough space -> Split the wild block
            // --------------------------------------------------
            Size new_wild_size = old_size - requested_block_size;
            assert(new_wild_size >= MIN_BLOCK_SIZE && new_wild_size % ALIGNMENT == 0);
            
            AllocBlockDesc allocated_size = { requested_block_size, (U32)AllocBlockState::USED, 0 };
            AllocBlockHeader* allocated = old_wild;
            allocated->this_desc = allocated_size;
            
            AllocBlockDesc new_wild_size_record = { new_wild_size, (U32)AllocBlockState::WILD_BLOCK, 0 };
            new_wild->this_desc = new_wild_size_record;
            new_wild->prev_desc = allocated->this_desc;
            global_kernel_state.alloc_table.wild_block = new_wild;
            next_block->prev_desc = new_wild->this_desc;
            
            // Update stats
            ++global_kernel_state.alloc_table.stats.alloc_counter[255];
            ++global_kernel_state.alloc_table.stats.split_counter[255];
            global_kernel_state.alloc_table.free_mem_size -= requested_block_size;
            
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
    /// \param side_coalescing Maximum number of merges per side (0 = no coalescing, defaults to UINT_MAX for unlimited).
    inline Void free_mem(Void* ptr, U32 side_coalescing) noexcept {
        using namespace priv;
        assert(ptr != nullptr);
        (Void)side_coalescing;

        // Release Block
        // -------------
        FreeAllocBlockHeader* block = (FreeAllocBlockHeader*)((Char*)ptr - HEADER_SIZE);
        AllocBlockDesc this_size = block->this_desc;
        Size block_size = this_size.size;

        // Update block state
        // -------------------
        AllocBlockState block_state = (AllocBlockState)this_size.state;
        assert(block_state == AllocBlockState::USED);        
        block->this_desc.state = (U32)AllocBlockState::FREE;

        // Update next block prevSize
        // --------------------------
        AllocBlockHeader* next_block = next((AllocBlockHeader*)block);
        next_block->prev_desc = block->this_desc;
        
        // Update stats
        // ------------
        unsigned orig_bin_idx = get_alloc_freelist_index(block_size);
        assert(orig_bin_idx < 255);
        utl::push_link(&global_kernel_state.alloc_table.freelist_head[orig_bin_idx], &block->freelist_link);
        ++global_kernel_state.alloc_table.stats.free_counter[orig_bin_idx];
        ++global_kernel_state.alloc_table.stats.pooled_counter[orig_bin_idx];
        ++global_kernel_state.alloc_table.freelist_count[orig_bin_idx];
        set_alloc_freelist_mask(&global_kernel_state.alloc_table.freelist_mask, orig_bin_idx);
        global_kernel_state.alloc_table.free_mem_size += block_size;
        
    }
}