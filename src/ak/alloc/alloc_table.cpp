#include "ak/alloc/alloc.hpp" // IWYU pragma: keep

namespace ak { 
    constexpr Size MAX_SMALL_BIN_SIZE = 2048;
    static constexpr Size HEADER_SIZE    = 16;
    static constexpr Size MIN_BLOCK_SIZE = 32;
    static constexpr Size ALIGNMENT      = 32;
    static constexpr int STATS_IDX_TREE = AllocStats::ALLOCATOR_BIN_COUNT;       // 64
    static constexpr int STATS_IDX_WILD = AllocStats::ALLOCATOR_BIN_COUNT + 1;   // 65

    const Char* to_string(AllocBlockState s) noexcept {
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

    I32 priv::init_alloc_table(AllocTable* at, Void* mem, Size size) noexcept {
        
        constexpr U64 SENTINEL_SIZE = sizeof(AllocPooledFreeBlockHeader);

        AK_ASSERT(mem != nullptr);
        AK_ASSERT(size >= 4096);

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
        AK_ASSERT(((U64)begin_sentinel       & 31ull) == 0ull);
        AK_ASSERT(((U64)wild_block           & 31ull) == 0ull);
        AK_ASSERT(((U64)end_sentinel         & 31ull) == 0ull);
        
        
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
            init_dlink(&at->freelist_head[i]);
        }
        at->freelist_count[63] = 0; // bin 63 is a regular freelist bin (up to 2048)
        at->freelist_mask = 0ull;
        check_alloc_table_invariants(at);
        return 0;
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
    Void* priv::try_alloc_table_malloc(AllocTable* at, Size size) noexcept {
        using namespace priv;
        check_alloc_table_invariants(at);
        // Compute aligned block size
        Size maybe_block = HEADER_SIZE + size;
        Size unaligned = maybe_block & (ALIGNMENT - 1);
        Size requested_block_size = (unaligned != 0) ? maybe_block + (ALIGNMENT - unaligned) : maybe_block;
        AK_ASSERT((requested_block_size & (ALIGNMENT - 1)) == 0);
        AK_ASSERT(requested_block_size >= MIN_BLOCK_SIZE);

        
        // Try small bin freelists first when eligible (<= 2048)
        I32 bin_idx = -1;
        if (requested_block_size <= MAX_SMALL_BIN_SIZE) {
            bin_idx = find_alloc_freelist_index(&at->freelist_mask, requested_block_size);
        }
        
        // Small bin allocation case (bins 0..63)
        // ======================================
        if (bin_idx >= 0) {
            AK_ASSERT(at->freelist_count[bin_idx] > 0);
            AK_ASSERT(get_alloc_freelist_mask(&at->freelist_mask, bin_idx));
            
            priv::DLink* free_stack = &at->freelist_head[bin_idx];
            priv::DLink* link = pop_dlink(free_stack);
            --at->freelist_count[bin_idx];
            if (at->freelist_count[bin_idx] == 0) {
                clear_alloc_freelist_mask(&at->freelist_mask, bin_idx);
            }
            AllocBlockHeader* block = (AllocBlockHeader*)((Char*)link - AK_OFFSET(AllocPooledFreeBlockHeader, freelist_link));
            AllocBlockHeader* next_block = next(block);
            __builtin_prefetch(next_block, 1, 3);
            
            if constexpr (IS_DEBUG_MODE) { clear_dlink(link); }

            Size block_size = block->this_desc.size;
            
            // Exact match case
            // ----------------
            if (block_size == requested_block_size) {  
                // Update This State
                AK_ASSERT(block->this_desc.state == (U32)AllocBlockState::FREE);
                block->this_desc.state = (U32)AllocBlockState::USED;
                AK_ASSERT(block->this_desc.state == (U32)AllocBlockState::USED);
                
                // Update Prev State
                AK_ASSERT(next_block->prev_desc.state == (U32)AllocBlockState::FREE);
                next_block->prev_desc.state = (U32)AllocBlockState::USED;
                AK_ASSERT(next_block->prev_desc.state == (U32)AllocBlockState::USED);

                at->free_mem_size -= requested_block_size;
                ++at->stats.alloc_counter[bin_idx];
                ++at->stats.reused_counter[bin_idx];
                
                check_alloc_table_invariants(at);
                return (Void*)((Char*)block + HEADER_SIZE);
            } 
            
            // Required Split case
            // -------------------
            
            Size new_free_size = block_size - requested_block_size;
            AK_ASSERT(new_free_size >= MIN_BLOCK_SIZE && new_free_size % ALIGNMENT == 0);
            
            // Prefetch the new free block
            // ----------------------------
            AllocPooledFreeBlockHeader* new_free = (AllocPooledFreeBlockHeader*)((Char*)block + requested_block_size);
            __builtin_prefetch(new_free, 1, 3);

            // Prefetch stats
            // --------------
            Size new_bin_idx = get_alloc_freelist_index(new_free_size);
            __builtin_prefetch(&at->stats.split_counter[bin_idx], 1, 3);  
            __builtin_prefetch(&at->stats.alloc_counter[bin_idx], 1, 3);
            __builtin_prefetch(&at->stats.pooled_counter[new_bin_idx],  1, 3);

            // Update the new free block
            // -------------------------
            AK_ASSERT(block->this_desc.state == (U32)AllocBlockState::FREE);

            AllocBlockDesc new_alloc_record_size = { requested_block_size, (U32)AllocBlockState::USED, 0 };
            block->this_desc   = new_alloc_record_size;
            new_free->prev_desc = new_alloc_record_size;

            AllocBlockDesc new_free_size_record = { new_free_size, (U32)AllocBlockState::FREE, 0 };
            new_free->this_desc   = new_free_size_record;
            next_block->prev_desc = new_free_size_record;
            
            AK_ASSERT(block->this_desc.state == (U32)AllocBlockState::USED);
            AK_ASSERT(next_block->prev_desc.state == (U32)AllocBlockState::FREE);
            AK_ASSERT(new_free->this_desc.state == (U32)AllocBlockState::FREE);

            // Update stats
            // ------------
            
            ++at->stats.split_counter[bin_idx];
            ++at->stats.alloc_counter[bin_idx];
            // push to head (LIFO)
            push_dlink(&at->freelist_head[new_bin_idx], &new_free->freelist_link);
            set_alloc_freelist_mask(&at->freelist_mask, new_bin_idx);
            ++at->stats.pooled_counter[new_bin_idx];            
            ++at->freelist_count[new_bin_idx];
            at->free_mem_size -= requested_block_size;
            
            return (Void*)((Char*)block + HEADER_SIZE);            
        }

        // Large block tree allocation path for sizes > 2048
        if (requested_block_size > MAX_SMALL_BIN_SIZE) {
            AllocFreeBlockHeader* free_block = find_gte_free_block(at->root_free_block, requested_block_size);
            if (free_block != nullptr) {
                // Detach chosen block from the tree/list structure
                detach_free_block(&at->root_free_block, free_block);

                AllocBlockHeader* block = (AllocBlockHeader*)free_block;
                AllocBlockHeader* next_block = next(block);
                __builtin_prefetch(next_block, 1, 3);

                Size block_size = block->this_desc.size;
                if (block_size == requested_block_size) {
                    // Exact match
                    AK_ASSERT(block->this_desc.state == (U32)AllocBlockState::FREE);
                    block->this_desc.state = (U32)AllocBlockState::USED;
                    AK_ASSERT(next_block->prev_desc.state == (U32)AllocBlockState::FREE);
                    next_block->prev_desc.state = (U32)AllocBlockState::USED;
                    at->free_mem_size -= requested_block_size;
                    // Count as large allocation under stats index TREE
                    ++at->stats.alloc_counter[STATS_IDX_TREE];
                    ++at->stats.reused_counter[STATS_IDX_TREE];
                    
                    check_alloc_table_invariants(at);
                    return (Void*)((Char*)block + HEADER_SIZE);
                }

                // Split large free block
                Size new_free_size = block_size - requested_block_size;
                AK_ASSERT(new_free_size >= MIN_BLOCK_SIZE && new_free_size % ALIGNMENT == 0);
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
                    put_free_block(&at->root_free_block, (AllocBlockHeader*)new_free_hdr);
                } else {
                    U32 new_bin_idx = get_alloc_freelist_index(new_free_size);
                    push_dlink(&at->freelist_head[new_bin_idx], &((AllocPooledFreeBlockHeader*)new_free_hdr)->freelist_link);
                    set_alloc_freelist_mask(&at->freelist_mask, new_bin_idx);
                    ++at->freelist_count[new_bin_idx];
                    ++at->stats.pooled_counter[new_bin_idx];
                }

                ++at->stats.alloc_counter[STATS_IDX_TREE];
                ++at->stats.split_counter[STATS_IDX_TREE];
                at->free_mem_size -= requested_block_size;

                check_alloc_table_invariants(at);
                return (Void*)((Char*)block + HEADER_SIZE);
            }
        }

        // Update the free block
        // ---------------------
        
        // Fallback: allocate from the Wild Block
        // ======================================
        {
            AK_ASSERT(at->wild_block != nullptr);                      // Wild block pointer always valid
            // No freelist bit for wild; use boundary bin 63 for accounting

            // Note: The wild block is a degenerate case; it does not use free bins
            //       and it must always be allocated; which means have at least MIN_BLOCK_SIZE free space
            
            AllocBlockHeader* old_wild = (AllocBlockHeader*)at->wild_block;            
            
            // Prefetch the next block, the prev block and the new wild block
            // --------------------------------------------------------------
            
            // 1. Prefetch the next block
            AllocBlockHeader* next_block = next(old_wild);
            __builtin_prefetch(next_block, 1, 3);
            
            // 2. Prefetch the new wild block
            AllocPooledFreeBlockHeader* new_wild = (AllocPooledFreeBlockHeader*)((Char*)old_wild + requested_block_size);
            __builtin_prefetch(new_wild, 1, 3);

            // 3. Prefetch stats
            __builtin_prefetch(&at->stats.alloc_counter[STATS_IDX_WILD], 1, 3);
            __builtin_prefetch(&at->stats.split_counter[STATS_IDX_WILD], 1, 3);
            
            // Case there the wild block is full; memory is exhausted
            // ------------------------------------------------------
            Size old_size = old_wild->this_desc.size;
            if (requested_block_size > old_size - MIN_BLOCK_SIZE) {
                // the wild block must have at least MIN_BLOCK_SIZE free space
                ++at->stats.failed_counter[STATS_IDX_WILD];
                return nullptr; // not enough space
            }
            
            // Case there is enough space -> Split the wild block
            // --------------------------------------------------
            Size new_wild_size = old_size - requested_block_size;
            AK_ASSERT(new_wild_size >= MIN_BLOCK_SIZE && new_wild_size % ALIGNMENT == 0);
            
            AllocBlockDesc allocated_size = { requested_block_size, (U32)AllocBlockState::USED, 0 };
            AllocBlockHeader* allocated = old_wild;
            allocated->this_desc = allocated_size;
            
            AllocBlockDesc new_wild_size_record = { new_wild_size, (U32)AllocBlockState::WILD_BLOCK, 0 };
            new_wild->this_desc = new_wild_size_record;
            new_wild->prev_desc = allocated->this_desc;
            at->wild_block = new_wild;
            next_block->prev_desc = new_wild->this_desc;
            
            // Update stats
            ++at->stats.alloc_counter[STATS_IDX_WILD];
            ++at->stats.split_counter[STATS_IDX_WILD];
            at->free_mem_size -= requested_block_size;
            
            check_alloc_table_invariants(at);
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
    Void priv::alloc_table_free(AllocTable* at, Void* ptr, U32 side_coalescing) noexcept {
        using namespace priv;
        AK_ASSERT(ptr != nullptr);
        (Void)side_coalescing;

        check_alloc_table_invariants(at);
        // Release Block
        // -------------
        AllocPooledFreeBlockHeader* block = (AllocPooledFreeBlockHeader*)((Char*)ptr - HEADER_SIZE);
        AllocBlockDesc this_size = block->this_desc;
        Size block_size = this_size.size;

        // Update block state
        // -------------------
        AllocBlockState block_state = (AllocBlockState)this_size.state;
        AK_ASSERT(block_state == AllocBlockState::USED);        
        block->this_desc.state = (U32)AllocBlockState::FREE;
        at->free_mem_size += block_size;

        // Update next block prevSize
        // --------------------------
        AllocBlockHeader* next_block = next((AllocBlockHeader*)block);
        next_block->prev_desc = block->this_desc;

        // Update stats
        // ------------

        // Place freed block back into appropriate structure
        if (block_size > MAX_SMALL_BIN_SIZE) {
            ak::priv::put_free_block(&at->root_free_block, (AllocBlockHeader*)block);
            ++at->stats.free_counter[STATS_IDX_TREE];
            check_alloc_table_invariants(at);
            return;
        }

        // Small bin free case (bins 0..63)
        // --------------------------------
        unsigned orig_bin_idx = get_alloc_freelist_index(block_size);
        AK_ASSERT(orig_bin_idx < AllocTable::ALLOCATOR_BIN_COUNT);
        // push to head of freelist (DLink)
        push_dlink(&at->freelist_head[orig_bin_idx], &block->freelist_link);
        ++at->stats.free_counter[orig_bin_idx];
        ++at->stats.pooled_counter[orig_bin_idx];
        ++at->freelist_count[orig_bin_idx];
        set_alloc_freelist_mask(&at->freelist_mask, orig_bin_idx);
        check_alloc_table_invariants(at);
    }


    // Coalesce helpers: merge adjacent free or wild blocks into the provided block
    // Returns: total merged size added into '*out_block' (not including original block size), or -1 on error
    I64 priv::coalesce_alloc_table_left(AllocTable* at, AllocBlockHeader** out_block, U32 max_merges) noexcept {
        AK_ASSERT(out_block != nullptr);
        AllocBlockHeader* block = *out_block;
        AK_ASSERT(block != nullptr);
        check_alloc_table_invariants(at);
        AllocBlockState st = (AllocBlockState)block->this_desc.state;
        if (!(st == AllocBlockState::FREE || st == AllocBlockState::WILD_BLOCK)) return -1;

        // Detach starting block if FREE
        if (st == AllocBlockState::FREE) {
            U64 sz = block->this_desc.size;
            if (sz <= MAX_SMALL_BIN_SIZE) {
                U32 bin = get_alloc_freelist_index(sz);
                priv::DLink* link = &((AllocPooledFreeBlockHeader*)block)->freelist_link;
                if (!is_dlink_detached(link)) {
                    detach_dlink(link);
                    AK_ASSERT(at->freelist_count[bin] > 0);
                    --at->freelist_count[bin];
                    if (at->freelist_count[bin] == 0) {
                        clear_alloc_freelist_mask(&at->freelist_mask, bin);
                    }
                }
            } else {
                detach_free_block(&at->root_free_block, (AllocFreeBlockHeader*)block);
            }
        }

        I64 merged = 0;
        while (max_merges--) {
            AllocBlockHeader* left = prev(block);
            AllocBlockState lst = (AllocBlockState)left->this_desc.state;
            if (!(lst == AllocBlockState::FREE || lst == AllocBlockState::WILD_BLOCK)) break;

            // Detach the left neighbor from free structures immediately
            U64 left_size = left->this_desc.size;
            if (lst == AllocBlockState::FREE) {
                if (left_size <= MAX_SMALL_BIN_SIZE) {
                    U32 lbin = get_alloc_freelist_index(left_size);
                    priv::DLink* link = &((AllocPooledFreeBlockHeader*)left)->freelist_link;
                    if (!is_dlink_detached(link)) {
                        detach_dlink(link);
                        AK_ASSERT(at->freelist_count[lbin] > 0);
                        --at->freelist_count[lbin];
                        if (at->freelist_count[lbin] == 0) {
                            clear_alloc_freelist_mask(&at->freelist_mask, lbin);
                        }
                    }
                    ++at->stats.merged_counter[lbin];
                } else {
                    detach_free_block(&at->root_free_block, (AllocFreeBlockHeader*)left);
                    ++at->stats.merged_counter[STATS_IDX_TREE];
                }
            } else { // WILD_BLOCK
                block->this_desc.state = (U32)AllocBlockState::WILD_BLOCK;
                at->wild_block = (AllocPooledFreeBlockHeader*)block;
                ++at->stats.merged_counter[STATS_IDX_WILD];
            }

            U64 cur_size  = block->this_desc.size;
            U64 new_size  = left_size + cur_size;
            block = left; // shift to left block
            block->this_desc.size = new_size;
            AllocBlockHeader* right = next((AllocBlockHeader*)block);
            right->prev_desc = block->this_desc;
            merged += (I64)left_size;
        }

        // Reinsert resulting block if it is FREE (not WILD)
        if ((AllocBlockState)block->this_desc.state == AllocBlockState::FREE) {
            U64 sz = block->this_desc.size;
            if (sz <= MAX_SMALL_BIN_SIZE) {
                U32 bin = get_alloc_freelist_index(sz);
                push_dlink(&at->freelist_head[bin], &((AllocPooledFreeBlockHeader*)block)->freelist_link);
                set_alloc_freelist_mask(&at->freelist_mask, bin);
                ++at->freelist_count[bin];
                ++at->stats.pooled_counter[bin];
                ++at->stats.free_counter[bin];
            } else {
                put_free_block(&at->root_free_block, (AllocBlockHeader*)block);
                ++at->stats.free_counter[STATS_IDX_TREE];
            }
        } else {
            // Ensure wild pointer is set
            at->wild_block = (AllocPooledFreeBlockHeader*)block;
        }

        *out_block = block;
        check_alloc_table_invariants(at);
        return merged;
    }

    I64 priv::coalesce_alloc_table_right(AllocTable* at, AllocBlockHeader** out_block, U32 max_merges) noexcept {
        AK_ASSERT(out_block != nullptr);
        AllocBlockHeader* block = *out_block;
        AK_ASSERT(block != nullptr);
        check_alloc_table_invariants(at);
        AllocBlockState st = (AllocBlockState)block->this_desc.state;
        if (!(st == AllocBlockState::FREE || st == AllocBlockState::WILD_BLOCK)) return -1;

        // Detach starting block if FREE
        if (st == AllocBlockState::FREE) {
            U64 sz = block->this_desc.size;
            if (sz <= MAX_SMALL_BIN_SIZE) {
                U32 bin = get_alloc_freelist_index(sz);
                priv::DLink* link = &((AllocPooledFreeBlockHeader*)block)->freelist_link;
                if (!is_dlink_detached(link)) {
                    detach_dlink(link);
                    AK_ASSERT(at->freelist_count[bin] > 0);
                    --at->freelist_count[bin];
                    if (at->freelist_count[bin] == 0) {
                        clear_alloc_freelist_mask(&at->freelist_mask, bin);
                    }
                }
            } else {
                detach_free_block(&at->root_free_block, (AllocFreeBlockHeader*)block);
            }
        }

        I64 merged = 0;
        while (max_merges--) {
            AllocBlockHeader* right = next(block);
            AllocBlockState rst = (AllocBlockState)right->this_desc.state;
            if (!(rst == AllocBlockState::FREE || rst == AllocBlockState::WILD_BLOCK)) break;

            U64 right_size = right->this_desc.size;
            if (rst == AllocBlockState::FREE) {
                if (right_size <= MAX_SMALL_BIN_SIZE) {
                    U32 rbin = get_alloc_freelist_index(right_size);
                    priv::DLink* link = &((AllocPooledFreeBlockHeader*)right)->freelist_link;
                    if (!is_dlink_detached(link)) {
                        detach_dlink(link);
                        AK_ASSERT(at->freelist_count[rbin] > 0);
                        --at->freelist_count[rbin];
                        if (at->freelist_count[rbin] == 0) {
                            clear_alloc_freelist_mask(&at->freelist_mask, rbin);
                        }
                    }
                    ++at->stats.merged_counter[rbin];
                } else {
                    detach_free_block(&at->root_free_block, (AllocFreeBlockHeader*)right);
                    ++at->stats.merged_counter[STATS_IDX_TREE];
                }
            } else { // WILD_BLOCK
                block->this_desc.state = (U32)AllocBlockState::WILD_BLOCK;
                at->wild_block = (AllocPooledFreeBlockHeader*)block;
                ++at->stats.merged_counter[STATS_IDX_WILD];
            }

            U64 cur_size   = block->this_desc.size;
            U64 new_size   = cur_size + right_size;
            block->this_desc.size = new_size;
            AllocBlockHeader* right_right = next(block);
            right_right->prev_desc = block->this_desc;
            merged += (I64)right_size;
        }

        // Reinsert resulting block if it is FREE (not WILD)
        if ((AllocBlockState)block->this_desc.state == AllocBlockState::FREE) {
            U64 sz = block->this_desc.size;
            if (sz <= MAX_SMALL_BIN_SIZE) {
                U32 bin = get_alloc_freelist_index(sz);
                push_dlink(&at->freelist_head[bin], &((AllocPooledFreeBlockHeader*)block)->freelist_link);
                set_alloc_freelist_mask(&at->freelist_mask, bin);
                ++at->freelist_count[bin];
                ++at->stats.pooled_counter[bin];
                ++at->stats.free_counter[bin];
            } else {
                put_free_block(&at->root_free_block, (AllocBlockHeader*)block);
                ++at->stats.free_counter[STATS_IDX_TREE];
            }
        } else {
            // Ensure wild pointer is set
            at->wild_block = (AllocPooledFreeBlockHeader*)block;
        }

        *out_block = block;
        check_alloc_table_invariants(at);
        return merged;
    }

    I32 priv::defrag_alloc_table_mem(AllocTable* at, U64 millis_budget) noexcept {
        (void)millis_budget;
        priv::check_alloc_table_invariants(at);
        using namespace priv;
        int defragged = 0;
        AllocBlockHeader* begin = (AllocBlockHeader*)at->sentinel_begin;
        AllocBlockHeader* end   = (AllocBlockHeader*)next((AllocBlockHeader*)at->sentinel_end);
        for (AllocBlockHeader* h = begin; h != end; h = next(h)) {
            AllocBlockState st = (AllocBlockState)h->this_desc.state;
            if (st != AllocBlockState::FREE) continue;
            AllocBlockHeader* cur = h;
            I64 merged = ak::priv::coalesce_alloc_table_right(at, &cur, 1);
            if (merged > 0) ++defragged;
            h = cur; // continue from the merged block
        }
        priv::check_alloc_table_invariants(at);
        return defragged;
    }
}
