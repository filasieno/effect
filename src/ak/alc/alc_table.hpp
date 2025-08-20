#pragma once

#include "ak/alc/alc_api_priv.hpp" // IWYU pragma: keep
#include "ak/alc/alc_freeblock_tree.hpp" // IWYU pragma: keep
#include "ak/alc/alc_freeblock_list.hpp" // coalesce helpers

namespace ak {
    // Stats helper indices to avoid magic numbers
    static constexpr int STATS_IDX_TREE = AllocStats::ALLOCATOR_BIN_COUNT;       // 64
    static constexpr int STATS_IDX_WILD = AllocStats::ALLOCATOR_BIN_COUNT + 1;   // 65
}

namespace ak { 

    inline const Char* to_string(AllocBlockState s) noexcept {
        switch (s) {
            case AllocBlockState::USED:                 return "USED";
            case AllocBlockState::FREE:                 return "FREE";
            case AllocBlockState::WILD_BLOCK:           return "WILD";
            case AllocBlockState::BEGIN_SENTINEL:       return "SENTINEL B";
            case AllocBlockState::LARGE_BLOCK_SENTINEL: return "SENTINEL L";
            case AllocBlockState::END_SENTINEL:         return "SENTINEL E";
            default:                                    return "INVALID";
        }
    }

    namespace priv {

   
    static constexpr Size HEADER_SIZE    = 16;
    static constexpr Size MIN_BLOCK_SIZE = 32;
    static constexpr Size ALIGNMENT      = 32;

    
    inline int init_alloc_table(Void* mem, Size size) noexcept {
        AllocTable* at = &global_kernel_state.alloc_table;
        
        constexpr U64 SENTINEL_SIZE = sizeof(AllocPooledFreeBlockHeader);

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
        // Layout: [BeginSentinel] ... blocks ... [EndSentinel]
        AllocPooledFreeBlockHeader* begin_sentinel      = (AllocPooledFreeBlockHeader*)aligned_begin;
        AllocPooledFreeBlockHeader* wild_block          = (AllocPooledFreeBlockHeader*)((Char*)begin_sentinel + SENTINEL_SIZE);
        AllocPooledFreeBlockHeader* end_sentinel        = (AllocPooledFreeBlockHeader*)((Char*)aligned_end    - SENTINEL_SIZE);
        // freelist links unused for wild block
        
        // Check alignments
        assert(((U64)begin_sentinel       & 31ull) == 0ull);
        assert(((U64)wild_block           & 31ull) == 0ull);
        assert(((U64)end_sentinel         & 31ull) == 0ull);
        
        
        at->sentinel_begin       = begin_sentinel;
        at->wild_block           = wild_block;
        at->sentinel_end         = end_sentinel;
        init_free_block_tree_root(&at->root_free_block);
        
        begin_sentinel->this_desc.size       = (U64)SENTINEL_SIZE;
        begin_sentinel->this_desc.state      = (U32)AllocBlockState::BEGIN_SENTINEL;
        // Initialize prevSize for the begin sentinel to avoid reading
        // uninitialized memory in debug printers.
        begin_sentinel->prev_desc             = { 0ull, (U32)AllocBlockState::INVALID, 0ull };
        wild_block->this_desc.size            = (U64)((U64)end_sentinel - (U64)wild_block);
        wild_block->this_desc.state           = (U32)AllocBlockState::WILD_BLOCK;
        end_sentinel->this_desc.size          = (U64)SENTINEL_SIZE;
        end_sentinel->this_desc.state         = (U32)AllocBlockState::END_SENTINEL;
        wild_block->prev_desc                 = begin_sentinel->this_desc;
        end_sentinel->prev_desc               = wild_block->this_desc;
        at->free_mem_size                     = wild_block->this_desc.size;

        for (int i = 0; i < AllocTable::ALLOCATOR_BIN_COUNT; ++i) {
            utl::init_dlink(&at->freelist_head[i]);
        }
        at->freelist_count[63] = 0; // bin 63 is a regular freelist bin (up to 2048)
        at->freelist_mask = 0ull;

        return 0;
    }


}} // namespace ak::priv



namespace ak { 

    constexpr Size MAX_SMALL_BIN_SIZE = 2048;
    
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

        
        // Try small bin freelists first when eligible (<= 2048)
        I32 bin_idx = -1;
        if (requested_block_size <= MAX_SMALL_BIN_SIZE) {
            bin_idx = find_alloc_freelist_index(&global_kernel_state.alloc_table.freelist_mask, requested_block_size);
        }
        
        // Small bin allocation case (bins 0..63)
        // ======================================
        if (bin_idx >= 0) {
            assert(global_kernel_state.alloc_table.freelist_count[bin_idx] > 0);
            assert(get_alloc_freelist_mask(&global_kernel_state.alloc_table.freelist_mask, bin_idx));
            
            utl::DLink* free_stack = &global_kernel_state.alloc_table.freelist_head[bin_idx];
            utl::DLink* link = utl::pop_dlink(free_stack);
            --global_kernel_state.alloc_table.freelist_count[bin_idx];
            if (global_kernel_state.alloc_table.freelist_count[bin_idx] == 0) {
                clear_alloc_freelist_mask(&global_kernel_state.alloc_table.freelist_mask, bin_idx);
            }
            AllocBlockHeader* block = (AllocBlockHeader*)((Char*)link - AK_OFFSET(AllocPooledFreeBlockHeader, freelist_link));
            AllocBlockHeader* next_block = next(block);
            __builtin_prefetch(next_block, 1, 3);
            
            if constexpr (IS_DEBUG_MODE) { utl::clear_dlink(link); }

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
            AllocPooledFreeBlockHeader* new_free = (AllocPooledFreeBlockHeader*)((Char*)block + requested_block_size);
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
            // push to head (LIFO)
            utl::push_dlink(&global_kernel_state.alloc_table.freelist_head[new_bin_idx], &new_free->freelist_link);
            set_alloc_freelist_mask(&global_kernel_state.alloc_table.freelist_mask, new_bin_idx);
            ++global_kernel_state.alloc_table.stats.pooled_counter[new_bin_idx];            
            ++global_kernel_state.alloc_table.freelist_count[new_bin_idx];
            global_kernel_state.alloc_table.free_mem_size -= requested_block_size;
            
            return (Void*)((Char*)block + HEADER_SIZE);            
        }

        // Large block tree allocation path for sizes > 2048
        if (requested_block_size > MAX_SMALL_BIN_SIZE) {
            AllocFreeBlockHeader* free_block = find_gte_free_block(global_kernel_state.alloc_table.root_free_block, requested_block_size);
            if (free_block != nullptr) {
                // Detach chosen block from the tree/list structure
                detach_free_block(&global_kernel_state.alloc_table.root_free_block, free_block);

                AllocBlockHeader* block = (AllocBlockHeader*)free_block;
                AllocBlockHeader* next_block = next(block);
                __builtin_prefetch(next_block, 1, 3);

                Size block_size = block->this_desc.size;
                if (block_size == requested_block_size) {
                    // Exact match
                    assert(block->this_desc.state == (U32)AllocBlockState::FREE);
                    block->this_desc.state = (U32)AllocBlockState::USED;
                    assert(next_block->prev_desc.state == (U32)AllocBlockState::FREE);
                    next_block->prev_desc.state = (U32)AllocBlockState::USED;
                    global_kernel_state.alloc_table.free_mem_size -= requested_block_size;
                    // Count as large allocation under stats index TREE
                    ++global_kernel_state.alloc_table.stats.alloc_counter[STATS_IDX_TREE];
                    ++global_kernel_state.alloc_table.stats.reused_counter[STATS_IDX_TREE];
                    return (Void*)((Char*)block + HEADER_SIZE);
                }

                // Split large free block
                Size new_free_size = block_size - requested_block_size;
                assert(new_free_size >= MIN_BLOCK_SIZE && new_free_size % ALIGNMENT == 0);
                AllocBlockHeader* new_free_hdr = (AllocBlockHeader*)((Char*)block + requested_block_size);
                __builtin_prefetch(new_free_hdr, 1, 3);

                AllocBlockDesc alloc_desc = { requested_block_size, (U32)AllocBlockState::USED, 0 };
                block->this_desc = alloc_desc;
                ((AllocBlockHeader*)new_free_hdr)->prev_desc = alloc_desc;

                AllocBlockDesc free_desc = { new_free_size, (U32)AllocBlockState::FREE, 0 };
                ((AllocBlockHeader*)new_free_hdr)->this_desc = free_desc;
                next_block->prev_desc = free_desc;

                // Place the remainder appropriately
                if (new_free_size > MAX_SMALL_BIN_SIZE) {
                    put_free_block(&global_kernel_state.alloc_table.root_free_block, (AllocBlockHeader*)new_free_hdr);
                } else {
                    U32 new_bin_idx = get_alloc_freelist_index(new_free_size);
                    utl::push_dlink(&global_kernel_state.alloc_table.freelist_head[new_bin_idx], &((AllocPooledFreeBlockHeader*)new_free_hdr)->freelist_link);
                    set_alloc_freelist_mask(&global_kernel_state.alloc_table.freelist_mask, new_bin_idx);
                    ++global_kernel_state.alloc_table.freelist_count[new_bin_idx];
                    ++global_kernel_state.alloc_table.stats.pooled_counter[new_bin_idx];
                }

                ++global_kernel_state.alloc_table.stats.alloc_counter[STATS_IDX_TREE];
                ++global_kernel_state.alloc_table.stats.split_counter[STATS_IDX_TREE];
                global_kernel_state.alloc_table.free_mem_size -= requested_block_size;
                return (Void*)((Char*)block + HEADER_SIZE);
            }
        }

        // Update the free block
        // ---------------------
        
        // Fallback: allocate from the Wild Block
        // ======================================
        {
            assert(global_kernel_state.alloc_table.wild_block != nullptr);                      // Wild block pointer always valid
            // No freelist bit for wild; use boundary bin 63 for accounting

            // Note: The wild block is a degenerate case; it does not use free bins
            //       and it must always be allocated; which means have at least MIN_BLOCK_SIZE free space
            
            AllocBlockHeader* old_wild = (AllocBlockHeader*)global_kernel_state.alloc_table.wild_block;            
            
            // Prefetch the next block, the prev block and the new wild block
            // --------------------------------------------------------------
            
            // 1. Prefetch the next block
            AllocBlockHeader* next_block = next(old_wild);
            __builtin_prefetch(next_block, 1, 3);
            
            // 2. Prefetch the new wild block
            AllocPooledFreeBlockHeader* new_wild = (AllocPooledFreeBlockHeader*)((Char*)old_wild + requested_block_size);
            __builtin_prefetch(new_wild, 1, 3);

            // 3. Prefetch stats
            __builtin_prefetch(&global_kernel_state.alloc_table.stats.alloc_counter[STATS_IDX_WILD], 1, 3);
            __builtin_prefetch(&global_kernel_state.alloc_table.stats.split_counter[STATS_IDX_WILD], 1, 3);
            
            // Case there the wild block is full; memory is exhausted
            // ------------------------------------------------------
            Size old_size = old_wild->this_desc.size;
            if (requested_block_size > old_size - MIN_BLOCK_SIZE) {
                // the wild block must have at least MIN_BLOCK_SIZE free space
                ++global_kernel_state.alloc_table.stats.failed_counter[STATS_IDX_WILD];
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
            ++global_kernel_state.alloc_table.stats.alloc_counter[STATS_IDX_WILD];
            ++global_kernel_state.alloc_table.stats.split_counter[STATS_IDX_WILD];
            global_kernel_state.alloc_table.free_mem_size -= requested_block_size;
            
            return (Void*)((Char*)allocated + HEADER_SIZE);
        }
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
        AllocPooledFreeBlockHeader* block = (AllocPooledFreeBlockHeader*)((Char*)ptr - HEADER_SIZE);
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

        // Always attempt right-side merge with wild block first (handled inside coalesce_right)
        if ((AllocBlockState)next_block->this_desc.state == AllocBlockState::WILD_BLOCK) {
            I64 merged = coalesce_right((AllocBlockHeader*)block, /*max_merges*/1);
            (void)merged;
            next_block = next((AllocBlockHeader*)block);
            next_block->prev_desc = block->this_desc;
            global_kernel_state.alloc_table.free_mem_size += block_size;
            return;
        }

        // Place freed block back into appropriate structure
        if (block_size > MAX_SMALL_BIN_SIZE) {
            ak::priv::put_free_block(&global_kernel_state.alloc_table.root_free_block, (AllocBlockHeader*)block);
            ++global_kernel_state.alloc_table.stats.free_counter[STATS_IDX_TREE];
            global_kernel_state.alloc_table.free_mem_size += block_size;
            return;
        }
        

        unsigned orig_bin_idx = get_alloc_freelist_index(block_size);
        assert(orig_bin_idx < AllocTable::ALLOCATOR_BIN_COUNT);
        // push to head of freelist (DLink)
        utl::push_dlink(&global_kernel_state.alloc_table.freelist_head[orig_bin_idx], &block->freelist_link);
        ++global_kernel_state.alloc_table.stats.free_counter[orig_bin_idx];
        ++global_kernel_state.alloc_table.stats.pooled_counter[orig_bin_idx];
        ++global_kernel_state.alloc_table.freelist_count[orig_bin_idx];
        set_alloc_freelist_mask(&global_kernel_state.alloc_table.freelist_mask, orig_bin_idx);
        global_kernel_state.alloc_table.free_mem_size += block_size;
        
    }


    // Coalesce helpers: merge adjacent free or wild blocks into the provided block
    // Returns: total merged size added into 'block' (not including original block size), or -1 on error
    inline I64 priv::coalesce_left(AllocBlockHeader** out_block, AllocBlockHeader* block, U32 max_merges) noexcept {
        assert(out_block != nullptr);
        assert(block != nullptr);
        AllocBlockState st = (AllocBlockState)block->this_desc.state;
        if (!(st == AllocBlockState::FREE || st == AllocBlockState::WILD_BLOCK)) return -1;

        I64 merged = 0;
        while (max_merges--) {
            AllocBlockHeader* left = prev(block);
            AllocBlockState lst = (AllocBlockState)left->this_desc.state;
            if (!(lst == AllocBlockState::FREE || lst == AllocBlockState::WILD_BLOCK)) break;

            U64 left_size = left->this_desc.size;
            U64 cur_size  = block->this_desc.size;
            U64 new_size  = left_size + cur_size;

            block = left; // shift to left block
            block->this_desc.size = new_size;
            AllocBlockHeader* right = next((AllocBlockHeader*)block);
            right->prev_desc = block->this_desc;
            merged += (I64)left_size;
        }
        *out_block = block;
        return merged;
    }

    inline I64 priv::coalesce_right(AllocBlockHeader* block, U32 max_merges) noexcept {
        assert(block != nullptr);
        AllocBlockState st = (AllocBlockState)block->this_desc.state;
        if (!(st == AllocBlockState::FREE || st == AllocBlockState::WILD_BLOCK)) return -1;

        I64 merged = 0;
        while (max_merges--) {
            AllocBlockHeader* right = next(block);
            AllocBlockState rst = (AllocBlockState)right->this_desc.state;
            if (!(rst == AllocBlockState::FREE || rst == AllocBlockState::WILD_BLOCK)) break;

            U64 right_size = right->this_desc.size;
            U64 cur_size   = block->this_desc.size;
            U64 new_size   = cur_size + right_size;
            // If merging into the wild block, promote current block to wild and update global pointer
            if (rst == AllocBlockState::WILD_BLOCK) {
                block->this_desc.state = (U32)AllocBlockState::WILD_BLOCK;
                global_kernel_state.alloc_table.wild_block = (AllocPooledFreeBlockHeader*)block;
            }
            block->this_desc.size = new_size;
            AllocBlockHeader* right_right = next(block);
            right_right->prev_desc = block->this_desc;
            merged += (I64)right_size;
        }
        return merged;
    }
}
