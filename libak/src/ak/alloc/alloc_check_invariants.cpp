#include "ak/alloc/alloc.hpp" // IWYU pragma: keep



AkVoid alloc_table_check_invariants(AkAllocTable* at, std::source_location loc) noexcept {
    if constexpr (AK_IS_DEBUG_MODE && AK_ENABLE_FULL_INVARIANT_CHECKS) {

        // Basic table invariants
        AK_ASSERT_AT(loc, at->heap_begin < at->mem_begin, "basic alloc table invariant failed");
        AK_ASSERT_AT(loc, at->mem_begin  < at->mem_end, "basic alloc table invariant failed");
        AK_ASSERT_AT(loc, at->mem_end    < at->heap_end, "basic alloc table invariant failed");
        AK_ASSERT_AT(loc, ((AkU64)at->mem_begin & 31ull) == 0ull, "basic alloc table invariant failed");
        AK_ASSERT_AT(loc, ((AkU64)at->mem_end   & 31ull) == 0ull, "basic alloc table invariant failed");
        AK_ASSERT_AT(loc, at->mem_size == (AkSize)(at->mem_end - at->mem_begin), "basic alloc table invariant failed");

        // Sentinels positioning invariants
        AK_ASSERT_AT(loc, (AkVoid*)at->sentinel_begin == (AkVoid*)at->mem_begin, "sentinal position invariant failed");
        AK_ASSERT_AT(loc, (AkU64)at->sentinel_begin->this_desc.size % 32ull == 0ull, "sentinal position invariant failed");
        AK_ASSERT_AT(loc, at->sentinel_begin->this_desc.state == (AkU32)AkAllocBlockState::BEGIN_SENTINEL, "sentinal position invariant failed");
        AK_ASSERT_AT(loc, at->sentinel_begin->prev_desc.size == 0ull, "sentinal position invariant failed");

        AkAllocPooledFreeBlockHeader* expected_end = (AkAllocPooledFreeBlockHeader*)((AkChar*)at->mem_end - sizeof(AkAllocPooledFreeBlockHeader));
        AK_ASSERT_AT(loc, (AkVoid*)at->sentinel_end == (AkVoid*)expected_end, "sentinal position invariant failed");
        AK_ASSERT_AT(loc, (AkU64)at->sentinel_end->this_desc.size % 32ull == 0ull, "sentinal position invariant failed");
        AK_ASSERT_AT(loc, at->sentinel_end->this_desc.state == (AkU32)AkAllocBlockState::END_SENTINEL, "sentinal position invariant failed");

        // Wild block basic invariants
        AK_ASSERT_AT(loc, at->wild_block != nullptr, "wild block invariant failed");
        AK_ASSERT_AT(loc, (AkChar*)at->wild_block >= at->mem_begin, "wild block invariant failed");
        AK_ASSERT_AT(loc, (AkChar*)at->wild_block <  at->mem_end, "wild block invariant failed");
        AK_ASSERT_AT(loc, ((AkU64)at->wild_block & 31ull) == 0ull, "wild block invariant failed");
        AK_ASSERT_AT(loc, at->wild_block->this_desc.state == (AkU32)AkAllocBlockState::WILD_BLOCK, "wild block invariant failed");

        // Scan heap blocks and verify local invariants
        AkU64 counted_free_bytes = 0ull;
        AkU64 counted_used_bytes = 0ull;
        AkU64 counted_wild_bytes = 0ull;
        (void)counted_wild_bytes;

        AkU64 small_free_count_bin[AkAllocTable::ALLOCATOR_BIN_COUNT] = {};
        AkU64 large_free_block_count = 0ull;
        AkU64 wild_block_instances = 0ull;

        const AkAllocBlockHeader* begin = (AkAllocBlockHeader*)at->sentinel_begin;
        const AkAllocBlockHeader* end   = (AkAllocBlockHeader*)((AkChar*)at->sentinel_end + at->sentinel_end->this_desc.size);

        const AkAllocBlockHeader* prev = nullptr;
        for (const AkAllocBlockHeader* h = begin; h != end; h = alloc_block_next((AkAllocBlockHeader*)h)) {
            // Address bounds and alignment
            AK_ASSERT_AT(loc, (AkChar*)h >= at->mem_begin, "heap block invariant failed");
            AK_ASSERT_AT(loc, (AkChar*)h <  at->mem_end, "heap block invariant failed");
            AK_ASSERT_AT(loc, ((AkU64)h & 31ull) == 0ull, "heap block invariant failed");

            // AkSize constraints
            AkU64 sz = h->this_desc.size;
            AK_ASSERT_AT(loc, sz >= sizeof(AkAllocBlockHeader), "heap block invariant failed");
            AK_ASSERT_AT(loc, (sz & 31ull) == 0ull, "heap block invariant failed");

            // Prev descriptor consistency
            if (prev) {
                AK_ASSERT_AT(loc, h->prev_desc.size  == prev->this_desc.size, "heap block invariant failed");
                AK_ASSERT_AT(loc, h->prev_desc.state == prev->this_desc.state, "heap block invariant failed");
                // Bidirectional linkage check
                AK_ASSERT_AT(loc, alloc_block_next((AkAllocBlockHeader*)prev) == h, "heap block invariant failed");
                AK_ASSERT_AT(loc, alloc_block_prev((AkAllocBlockHeader*)h) == prev, "heap block invariant failed");
            } else {
                // First block is the begin sentinel
                AK_ASSERT_AT(loc, h == begin, "heap block invariant failed");
                AK_ASSERT_AT(loc, h->this_desc.state == (AkU32)AkAllocBlockState::BEGIN_SENTINEL, "heap block invariant failed");
            }

            // State-specific checks and accounting
            AkAllocBlockState st = (AkAllocBlockState)h->this_desc.state;
            switch (st) {
                case AkAllocBlockState::BEGIN_SENTINEL:
                    AK_ASSERT_AT(loc, h == begin, "heap block invariant failed");
                    counted_used_bytes += sz;
                    break;
                case AkAllocBlockState::END_SENTINEL:
                    AK_ASSERT_AT(loc, h == (AkAllocBlockHeader*)at->sentinel_end, "heap block invariant failed");
                    counted_used_bytes += sz;
                    break;
                case AkAllocBlockState::WILD_BLOCK:
                    AK_ASSERT_AT(loc, (AkAllocBlockHeader*)h == (AkAllocBlockHeader*)at->wild_block, "heap block invariant failed");
                    AK_ASSERT_AT(loc, sz >= 32ull, "heap block invariant failed");
                    ++wild_block_instances;
                    counted_wild_bytes += sz;
                    counted_free_bytes += sz;
                    break;
                case AkAllocBlockState::FREE:
                    AK_ASSERT(sz >= 32ull);
                    if (sz <= 2048ull) {
                        ++small_free_count_bin[alloc_freelist_get_index(h)];
                    } else {
                        ++large_free_block_count;
                    }
                    counted_free_bytes += sz;
                    break;
                case AkAllocBlockState::USED:
                    counted_used_bytes += sz;
                    break;
                default:
                    std::abort();
            }

            prev = h;
        }

        // Ensure we saw exactly one wild block instance
        AK_ASSERT_AT(loc, wild_block_instances == 1ull, "wild block invariant failed: {}", wild_block_instances);

        // Memory accounting: free + used should equal mem_size
        AK_ASSERT_AT(loc, counted_free_bytes + counted_used_bytes == at->mem_size, "memory accounting invariant failed: {} + {} != {}", counted_free_bytes, counted_used_bytes, at->mem_size);
        AK_ASSERT_AT(loc, counted_free_bytes == at->free_mem_size, "memory accounting invariant failed: {} != {}", counted_free_bytes, at->free_mem_size);

        // Validate small freelist structures: mask and counts
        AkU64 observed_mask = 0ull;
        for (AkU32 bin = 0; bin < AkAllocTable::ALLOCATOR_BIN_COUNT; ++bin) {
            AkDLink* head = &at->freelist_head[bin];
            AkU64 ring_count = 0ull;
            for (AkDLink* it = head->next; it != head; it = it->next) {
                const AkSize link_off = AK_OFFSET(AkAllocPooledFreeBlockHeader, freelist_link);
                AkAllocBlockHeader* b = (AkAllocBlockHeader*)((AkChar*)it - link_off);
                // Each member must be FREE and in-range
                AK_ASSERT_AT(loc, b->this_desc.state == (AkU32)AkAllocBlockState::FREE, "small freelist invariant failed: {}", to_string((AkAllocBlockState)(b->this_desc.state)));
                AK_ASSERT_AT(loc, b->this_desc.size <= 2048ull, "small freelist invariant failed");
                AK_ASSERT_AT(loc, alloc_freelist_get_index(b) == bin, "small freelist invariant failed");
                ++ring_count;
            }
            if (ring_count > 0) observed_mask |= (1ull << bin);
            AK_ASSERT_AT(loc, ring_count == (AkU64)at->freelist_count[bin], "small freelist invariant failed");
            AK_ASSERT_AT(loc, ring_count == small_free_count_bin[bin], "small freelist invariant failed");
            const AkBool mask_bit = ((at->freelist_mask >> bin) & 1ull) != 0ull;
            AK_ASSERT_AT(loc, mask_bit == (ring_count > 0), "small freelist invariant failed");
        }
        AK_ASSERT_AT(loc, observed_mask == at->freelist_mask, "small freelist invariant failed");

        // Validate large free block AVL tree: states and ordering
        AkU64 observed_large_free_count = 0ull;
        auto validate_tree = [&](auto&& self, AkAllocFreeBlockHeader* node, AkU64 min_key, AkU64 max_key) -> AkI32 {
            if (!node) return 0;
            AkU64 key = node->this_desc.size;
            AK_ASSERT_AT(loc, key > 2048ull, "large freelist invariant failed");
            AK_ASSERT_AT(loc, key > min_key && key < max_key, "large freelist invariant failed");
            AK_ASSERT_AT(loc, node->this_desc.state == (AkU32)AkAllocBlockState::FREE, "large freelist invariant failed");
            // children parent pointers
            if (node->left)  AK_ASSERT_AT(loc, node->left->parent  == node, "large freelist invariant failed");
            if (node->right) AK_ASSERT_AT(loc, node->right->parent == node, "large freelist invariant failed");
            // left subtree
            AkI32 hl = self(self, node->left, min_key, key);
            // right subtree
            AkI32 hr = self(self, node->right, key, max_key);
            // multimap ring: all nodes must have the same size and FREE state
            AkU64 list_count = 0ull;
            for (AkDLink* it = node->multimap_link.next; it != &node->multimap_link; it = it->next) {
                AkAllocFreeBlockHeader* n = (AkAllocFreeBlockHeader*)((AkChar*)it - AK_OFFSET(AkAllocFreeBlockHeader, multimap_link));
                AK_ASSERT_AT(loc, n->this_desc.size == key, "large freelist invariant failed");
                AK_ASSERT_AT(loc, n->this_desc.state == (AkU32)AkAllocBlockState::FREE, "large freelist invariant failed");
                ++list_count;
            }
            observed_large_free_count += 1ull + list_count;
            // AVL balance property based on computed heights
            AkI32 height = 1 + (hl > hr ? hl : hr);
            AkI32 balance = hl - hr;
            AK_ASSERT_AT(loc, balance >= -1 && balance <= 1, "large freelist invariant failed");
            (void)list_count;
            return height;
        };
        if (at->root_free_block) {
            (void)validate_tree(validate_tree, at->root_free_block, 2048ull, ~0ull);
        }
        AK_ASSERT_AT(loc, observed_large_free_count == large_free_block_count, "large freelist invariant failed");
    }
    }
