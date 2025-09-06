#include "ak/alloc/alloc.hpp" // IWYU pragma: keep



void alloc_table_check_invariants(ak_alloc_table* at, std::source_location loc) noexcept {
    if constexpr (AK_IS_DEBUG_MODE && AK_ENABLE_FULL_INVARIANT_CHECKS) {

        // Basic table invariants
        AK_ASSERT_AT(loc, at->heap_begin < at->mem_begin, "basic alloc table invariant failed");
        AK_ASSERT_AT(loc, at->mem_begin  < at->mem_end, "basic alloc table invariant failed");
        AK_ASSERT_AT(loc, at->mem_end    < at->heap_end, "basic alloc table invariant failed");
        AK_ASSERT_AT(loc, ((AkU64)at->mem_begin & 31ull) == 0ull, "basic alloc table invariant failed");
        AK_ASSERT_AT(loc, ((AkU64)at->mem_end   & 31ull) == 0ull, "basic alloc table invariant failed");
        AK_ASSERT_AT(loc, at->mem_size == (AkSize)(at->mem_end - at->mem_begin), "basic alloc table invariant failed");

        // Sentinels positioning invariants
        AK_ASSERT_AT(loc, (void*)at->sentinel_begin == (void*)at->mem_begin, "sentinal position invariant failed");
        AK_ASSERT_AT(loc, (AkU64)at->sentinel_begin->this_desc.size % 32ull == 0ull, "sentinal position invariant failed");
        AK_ASSERT_AT(loc, at->sentinel_begin->this_desc.state == (AkU32)AK_ALLOC_BLOCK_STATE_BEGIN_SENTINEL, "sentinal position invariant failed");
        AK_ASSERT_AT(loc, at->sentinel_begin->prev_desc.size == 0ull, "sentinal position invariant failed");

        AkAllocPooledFreeBlockHeader* expected_end = (AkAllocPooledFreeBlockHeader*)((char*)at->mem_end - sizeof(AkAllocPooledFreeBlockHeader));
        AK_ASSERT_AT(loc, (void*)at->sentinel_end == (void*)expected_end, "sentinal position invariant failed");
        AK_ASSERT_AT(loc, (AkU64)at->sentinel_end->this_desc.size % 32ull == 0ull, "sentinal position invariant failed");
        AK_ASSERT_AT(loc, at->sentinel_end->this_desc.state == (AkU32)AK_ALLOC_BLOCK_STATE_END_SENTINEL, "sentinal position invariant failed");

        // Wild block basic invariants
        AK_ASSERT_AT(loc, at->wild_block != nullptr, "wild block invariant failed");
        AK_ASSERT_AT(loc, (char*)at->wild_block >= at->mem_begin, "wild block invariant failed");
        AK_ASSERT_AT(loc, (char*)at->wild_block <  at->mem_end, "wild block invariant failed");
        AK_ASSERT_AT(loc, ((AkU64)at->wild_block & 31ull) == 0ull, "wild block invariant failed");
        AK_ASSERT_AT(loc, at->wild_block->this_desc.state == (AkU32)AK_ALLOC_BLOCK_STATE_WILD_BLOCK, "wild block invariant failed");

        // Scan heap blocks and verify local invariants
        AkU64 counted_free_bytes = 0ull;
        AkU64 counted_used_bytes = 0ull;
        AkU64 counted_wild_bytes = 0ull;
        (void)counted_wild_bytes;

        AkU64 small_free_count_bin[ak_alloc_table::ALLOCATOR_BIN_COUNT] = {};
        AkU64 large_free_block_count = 0ull;
        AkU64 wild_block_instances = 0ull;

        const ak_alloc_block_header* begin = (ak_alloc_block_header*)at->sentinel_begin;
        const ak_alloc_block_header* end   = (ak_alloc_block_header*)((char*)at->sentinel_end + at->sentinel_end->this_desc.size);

        const ak_alloc_block_header* prev = nullptr;
        for (const ak_alloc_block_header* h = begin; h != end; h = alloc_block_next((ak_alloc_block_header*)h)) {
            // Address bounds and alignment
            AK_ASSERT_AT(loc, (char*)h >= at->mem_begin, "heap block invariant failed");
            AK_ASSERT_AT(loc, (char*)h <  at->mem_end, "heap block invariant failed");
            AK_ASSERT_AT(loc, ((AkU64)h & 31ull) == 0ull, "heap block invariant failed");

            // AkSize constraints
            AkU64 sz = h->this_desc.size;
            AK_ASSERT_AT(loc, sz >= sizeof(ak_alloc_block_header), "heap block invariant failed");
            AK_ASSERT_AT(loc, (sz & 31ull) == 0ull, "heap block invariant failed");

            // Prev descriptor consistency
            if (prev) {
                AK_ASSERT_AT(loc, h->prev_desc.size  == prev->this_desc.size, "heap block invariant failed");
                AK_ASSERT_AT(loc, h->prev_desc.state == prev->this_desc.state, "heap block invariant failed");
                // Bidirectional linkage check
                AK_ASSERT_AT(loc, alloc_block_next((ak_alloc_block_header*)prev) == h, "heap block invariant failed");
                AK_ASSERT_AT(loc, alloc_block_prev((ak_alloc_block_header*)h) == prev, "heap block invariant failed");
            } else {
                // First block is the begin sentinel
                AK_ASSERT_AT(loc, h == begin, "heap block invariant failed");
                AK_ASSERT_AT(loc, h->this_desc.state == (AkU32)AK_ALLOC_BLOCK_STATE_BEGIN_SENTINEL, "heap block invariant failed");
            }

            // State-specific checks and accounting
            ak_alloc_block_state st = (ak_alloc_block_state)h->this_desc.state;
            switch (st) {
                case AK_ALLOC_BLOCK_STATE_BEGIN_SENTINEL:
                    AK_ASSERT_AT(loc, h == begin, "heap block invariant failed");
                    counted_used_bytes += sz;
                    break;
                case AK_ALLOC_BLOCK_STATE_END_SENTINEL:
                    AK_ASSERT_AT(loc, h == (ak_alloc_block_header*)at->sentinel_end, "heap block invariant failed");
                    counted_used_bytes += sz;
                    break;
                case AK_ALLOC_BLOCK_STATE_WILD_BLOCK:
                    AK_ASSERT_AT(loc, (ak_alloc_block_header*)h == (ak_alloc_block_header*)at->wild_block, "heap block invariant failed");
                    AK_ASSERT_AT(loc, sz >= 32ull, "heap block invariant failed");
                    ++wild_block_instances;
                    counted_wild_bytes += sz;
                    counted_free_bytes += sz;
                    break;
                case AK_ALLOC_BLOCK_STATE_FREE:
                    AK_ASSERT(sz >= 32ull);
                    if (sz <= 2048ull) {
                        ++small_free_count_bin[alloc_freelist_get_index(h)];
                    } else {
                        ++large_free_block_count;
                    }
                    counted_free_bytes += sz;
                    break;
                case AK_ALLOC_BLOCK_STATE_USED:
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
        for (AkU32 bin = 0; bin < ak_alloc_table::ALLOCATOR_BIN_COUNT; ++bin) {
            ak_dlink* head = &at->freelist_head[bin];
            AkU64 ring_count = 0ull;
            for (ak_dlink* it = head->next; it != head; it = it->next) {
                const AkSize link_off = AK_OFFSET(AkAllocPooledFreeBlockHeader, freelist_link);
                ak_alloc_block_header* b = (ak_alloc_block_header*)((char*)it - link_off);
                // Each member must be FREE and in-range
                AK_ASSERT_AT(loc, b->this_desc.state == (AkU32)AK_ALLOC_BLOCK_STATE_FREE, "small freelist invariant failed: {}", to_string((ak_alloc_block_state)(b->this_desc.state)));
                AK_ASSERT_AT(loc, b->this_desc.size <= 2048ull, "small freelist invariant failed");
                AK_ASSERT_AT(loc, alloc_freelist_get_index(b) == bin, "small freelist invariant failed");
                ++ring_count;
            }
            if (ring_count > 0) observed_mask |= (1ull << bin);
            AK_ASSERT_AT(loc, ring_count == (AkU64)at->freelist_count[bin], "small freelist invariant failed");
            AK_ASSERT_AT(loc, ring_count == small_free_count_bin[bin], "small freelist invariant failed");
            const bool mask_bit = ((at->freelist_mask >> bin) & 1ull) != 0ull;
            AK_ASSERT_AT(loc, mask_bit == (ring_count > 0), "small freelist invariant failed");
        }
        AK_ASSERT_AT(loc, observed_mask == at->freelist_mask, "small freelist invariant failed");

        // Validate large free block AVL tree: states and ordering
        AkU64 observed_large_free_count = 0ull;
        auto validate_tree = [&](auto&& self, ak_alloc_free_block_header* node, AkU64 min_key, AkU64 max_key) -> int {
            if (!node) return 0;
            AkU64 key = node->this_desc.size;
            AK_ASSERT_AT(loc, key > 2048ull, "large freelist invariant failed");
            AK_ASSERT_AT(loc, key > min_key && key < max_key, "large freelist invariant failed");
            AK_ASSERT_AT(loc, node->this_desc.state == (AkU32)AK_ALLOC_BLOCK_STATE_FREE, "large freelist invariant failed");
            // children parent pointers
            if (node->left)  AK_ASSERT_AT(loc, node->left->parent  == node, "large freelist invariant failed");
            if (node->right) AK_ASSERT_AT(loc, node->right->parent == node, "large freelist invariant failed");
            // left subtree
            int hl = self(self, node->left, min_key, key);
            // right subtree
            int hr = self(self, node->right, key, max_key);
            // multimap ring: all nodes must have the same size and FREE state
            AkU64 list_count = 0ull;
            for (ak_dlink* it = node->multimap_link.next; it != &node->multimap_link; it = it->next) {
                ak_alloc_free_block_header* n = (ak_alloc_free_block_header*)((char*)it - AK_OFFSET(ak_alloc_free_block_header, multimap_link));
                AK_ASSERT_AT(loc, n->this_desc.size == key, "large freelist invariant failed");
                AK_ASSERT_AT(loc, n->this_desc.state == (AkU32)AK_ALLOC_BLOCK_STATE_FREE, "large freelist invariant failed");
                ++list_count;
            }
            observed_large_free_count += 1ull + list_count;
            // AVL balance property based on computed heights
            int height = 1 + (hl > hr ? hl : hr);
            int balance = hl - hr;
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
