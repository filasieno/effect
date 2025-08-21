#include "ak/alloc/alloc_api.hpp"

namespace ak { namespace priv {

    Void check_alloc_table_invariants(std::source_location loc) noexcept {
        if constexpr (IS_DEBUG_MODE && ENABLE_FULL_INVARIANT_CHECKS) {
            AllocTable* at = &global_kernel_state.alloc_table;

            // Basic table invariants
            AK_ASSERT_AT(loc, at->heap_begin < at->mem_begin, "basic alloc table invariant failed");
            AK_ASSERT_AT(loc, at->mem_begin  < at->mem_end, "basic alloc table invariant failed");
            AK_ASSERT_AT(loc, at->mem_end    < at->heap_end, "basic alloc table invariant failed");
            AK_ASSERT_AT(loc, ((U64)at->mem_begin & 31ull) == 0ull, "basic alloc table invariant failed");
            AK_ASSERT_AT(loc, ((U64)at->mem_end   & 31ull) == 0ull, "basic alloc table invariant failed");
            AK_ASSERT_AT(loc, at->mem_size == (Size)(at->mem_end - at->mem_begin), "basic alloc table invariant failed");

            // Sentinels positioning invariants
            AK_ASSERT_AT(loc, (Void*)at->sentinel_begin == (Void*)at->mem_begin, "sentinal position invariant failed");
            AK_ASSERT_AT(loc, (U64)at->sentinel_begin->this_desc.size % 32ull == 0ull, "sentinal position invariant failed");
            AK_ASSERT_AT(loc, at->sentinel_begin->this_desc.state == (U32)AllocBlockState::BEGIN_SENTINEL, "sentinal position invariant failed");
            AK_ASSERT_AT(loc, at->sentinel_begin->prev_desc.size == 0ull, "sentinal position invariant failed");

            AllocPooledFreeBlockHeader* expected_end = (AllocPooledFreeBlockHeader*)((Char*)at->mem_end - sizeof(AllocPooledFreeBlockHeader));
            AK_ASSERT_AT(loc, (Void*)at->sentinel_end == (Void*)expected_end, "sentinal position invariant failed");
            AK_ASSERT_AT(loc, (U64)at->sentinel_end->this_desc.size % 32ull == 0ull, "sentinal position invariant failed");
            AK_ASSERT_AT(loc, at->sentinel_end->this_desc.state == (U32)AllocBlockState::END_SENTINEL, "sentinal position invariant failed");

            // Wild block basic invariants
            AK_ASSERT_AT(loc, at->wild_block != nullptr, "wild block invariant failed");
            AK_ASSERT_AT(loc, (Char*)at->wild_block >= at->mem_begin, "wild block invariant failed");
            AK_ASSERT_AT(loc, (Char*)at->wild_block <  at->mem_end, "wild block invariant failed");
            AK_ASSERT_AT(loc, ((U64)at->wild_block & 31ull) == 0ull, "wild block invariant failed");
            AK_ASSERT_AT(loc, at->wild_block->this_desc.state == (U32)AllocBlockState::WILD_BLOCK, "wild block invariant failed");

            // Scan heap blocks and verify local invariants
            U64 counted_free_bytes = 0ull;
            U64 counted_used_bytes = 0ull;
            U64 counted_wild_bytes = 0ull;
            (void)counted_wild_bytes;

            U64 small_free_count_bin[AllocTable::ALLOCATOR_BIN_COUNT] = {};
            U64 large_free_block_count = 0ull;
            U64 wild_block_instances = 0ull;

            const AllocBlockHeader* begin = (AllocBlockHeader*)at->sentinel_begin;
            const AllocBlockHeader* end   = (AllocBlockHeader*)((Char*)at->sentinel_end + at->sentinel_end->this_desc.size);

            const AllocBlockHeader* prev = nullptr;
            for (const AllocBlockHeader* h = begin; h != end; h = priv::next((AllocBlockHeader*)h)) {
                // Address bounds and alignment
                AK_ASSERT_AT(loc, (Char*)h >= at->mem_begin, "heap block invariant failed");
                AK_ASSERT_AT(loc, (Char*)h <  at->mem_end, "heap block invariant failed");
                AK_ASSERT_AT(loc, ((U64)h & 31ull) == 0ull, "heap block invariant failed");

                // Size constraints
                U64 sz = h->this_desc.size;
                AK_ASSERT_AT(loc, sz >= sizeof(AllocBlockHeader), "heap block invariant failed");
                AK_ASSERT_AT(loc, (sz & 31ull) == 0ull, "heap block invariant failed");

                // Prev descriptor consistency
                if (prev) {
                    AK_ASSERT_AT(loc, h->prev_desc.size  == prev->this_desc.size, "heap block invariant failed");
                    AK_ASSERT_AT(loc, h->prev_desc.state == prev->this_desc.state, "heap block invariant failed");
                    // Bidirectional linkage check
                    AK_ASSERT_AT(loc, priv::next((AllocBlockHeader*)prev) == h, "heap block invariant failed");
                    AK_ASSERT_AT(loc, priv::prev((AllocBlockHeader*)h) == prev, "heap block invariant failed");
                } else {
                    // First block is the begin sentinel
                    AK_ASSERT_AT(loc, h == begin, "heap block invariant failed");
                    AK_ASSERT_AT(loc, h->this_desc.state == (U32)AllocBlockState::BEGIN_SENTINEL, "heap block invariant failed");
                }

                // State-specific checks and accounting
                AllocBlockState st = (AllocBlockState)h->this_desc.state;
                switch (st) {
                    case AllocBlockState::BEGIN_SENTINEL:
                        AK_ASSERT_AT(loc, h == begin, "heap block invariant failed");
                        counted_used_bytes += sz;
                        break;
                    case AllocBlockState::END_SENTINEL:
                        AK_ASSERT_AT(loc, h == (AllocBlockHeader*)at->sentinel_end, "heap block invariant failed");
                        counted_used_bytes += sz;
                        break;
                    case AllocBlockState::WILD_BLOCK:
                        AK_ASSERT_AT(loc, (AllocBlockHeader*)h == (AllocBlockHeader*)at->wild_block, "heap block invariant failed");
                        AK_ASSERT_AT(loc, sz >= 32ull, "heap block invariant failed");
                        ++wild_block_instances;
                        counted_wild_bytes += sz;
                        counted_free_bytes += sz;
                        break;
                    case AllocBlockState::FREE:
                        AK_ASSERT(sz >= 32ull);
                        if (sz <= 2048ull) {
                            ++small_free_count_bin[priv::get_alloc_freelist_index(h)];
                        } else {
                            ++large_free_block_count;
                        }
                        counted_free_bytes += sz;
                        break;
                    case AllocBlockState::USED:
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
            U64 observed_mask = 0ull;
            for (U32 bin = 0; bin < AllocTable::ALLOCATOR_BIN_COUNT; ++bin) {
                priv::DLink* head = &at->freelist_head[bin];
                U64 ring_count = 0ull;
                for (priv::DLink* it = head->next; it != head; it = it->next) {
                    const Size link_off = AK_OFFSET(AllocPooledFreeBlockHeader, freelist_link);
                    AllocBlockHeader* b = (AllocBlockHeader*)((Char*)it - link_off);
                    // Each member must be FREE and in-range
                    AK_ASSERT_AT(loc, b->this_desc.state == (U32)AllocBlockState::FREE, "small freelist invariant failed: {}", to_string((AllocBlockState)(b->this_desc.state)));
                    AK_ASSERT_AT(loc, b->this_desc.size <= 2048ull, "small freelist invariant failed");
                    AK_ASSERT_AT(loc, priv::get_alloc_freelist_index(b) == bin, "small freelist invariant failed");
                    ++ring_count;
                }
                if (ring_count > 0) observed_mask |= (1ull << bin);
                AK_ASSERT_AT(loc, ring_count == (U64)at->freelist_count[bin], "small freelist invariant failed");
                AK_ASSERT_AT(loc, ring_count == small_free_count_bin[bin], "small freelist invariant failed");
                const Bool mask_bit = ((at->freelist_mask >> bin) & 1ull) != 0ull;
                AK_ASSERT_AT(loc, mask_bit == (ring_count > 0), "small freelist invariant failed");
            }
            AK_ASSERT_AT(loc, observed_mask == at->freelist_mask, "small freelist invariant failed");

            // Validate large free block AVL tree: states and ordering
            U64 observed_large_free_count = 0ull;
            auto validate_tree = [&](auto&& self, AllocFreeBlockHeader* node, U64 min_key, U64 max_key) -> I32 {
                if (!node) return 0;
                U64 key = node->this_desc.size;
                AK_ASSERT_AT(loc, key > 2048ull, "large freelist invariant failed");
                AK_ASSERT_AT(loc, key > min_key && key < max_key, "large freelist invariant failed");
                AK_ASSERT_AT(loc, node->this_desc.state == (U32)AllocBlockState::FREE, "large freelist invariant failed");
                // children parent pointers
                if (node->left)  AK_ASSERT_AT(loc, node->left->parent  == node, "large freelist invariant failed");
                if (node->right) AK_ASSERT_AT(loc, node->right->parent == node, "large freelist invariant failed");
                // left subtree
                I32 hl = self(self, node->left, min_key, key);
                // right subtree
                I32 hr = self(self, node->right, key, max_key);
                // multimap ring: all nodes must have the same size and FREE state
                U64 list_count = 0ull;
                for (priv::DLink* it = node->multimap_link.next; it != &node->multimap_link; it = it->next) {
                    AllocFreeBlockHeader* n = (AllocFreeBlockHeader*)((Char*)it - AK_OFFSET(AllocFreeBlockHeader, multimap_link));
                    AK_ASSERT_AT(loc, n->this_desc.size == key, "large freelist invariant failed");
                    AK_ASSERT_AT(loc, n->this_desc.state == (U32)AllocBlockState::FREE, "large freelist invariant failed");
                    ++list_count;
                }
                observed_large_free_count += 1ull + list_count;
                // AVL balance property based on computed heights
                I32 height = 1 + (hl > hr ? hl : hr);
                I32 balance = hl - hr;
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

}} // namespace ak::priv


