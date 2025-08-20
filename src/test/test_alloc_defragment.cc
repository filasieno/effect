#define AK_IMPLEMENTATION
#include "ak.hpp" // IWYU pragma: keep
#include <cassert>
#include <print>
#include <functional>

using namespace ak;
using namespace ak::priv;

static inline U64 sum_freelist_nodes() {
    U64 s = 0;
    for (int i = 0; i < AllocTable::ALLOCATOR_BIN_COUNT; ++i) s += global_kernel_state.alloc_table.freelist_count[i];
    return s;
}

void run_test(std::function<void()> block) noexcept {
    U64 buffer_size = 1024 * 1024;
    Void* buffer = std::malloc(buffer_size);
    KernelConfig cfg{ .mem = buffer, .memSize = buffer_size, .ioEntryCount = 256 };
    int init_rc = init_kernel(&cfg);
    assert(init_rc == 0);
    block();
    fini_kernel();
    std::free(buffer);
}

int main() noexcept {
    // Scenario 1: small block into small block (defragment merges two neighbors into one freelist node)
    run_test([]{
        Void* p1 = try_alloc_mem(32);
        Void* p2 = try_alloc_mem(32);
        assert(p1 && p2);
        free_mem(p1);
        free_mem(p2);

        U64 before_nodes = sum_freelist_nodes();
        I32 defrag = defragment_mem(0);
        U64 after_nodes = sum_freelist_nodes();
        assert(defrag >= 1);
        assert(after_nodes + 1 == before_nodes);
    });

    // Scenario 2: small block into wild block (free path already merges; defrag should do nothing)
    run_test([]{
        const auto& at = global_kernel_state.alloc_table;
        U64 merged_before = at.stats.merged_counter[STATS_IDX_WILD];

        Void* p = try_alloc_mem(64);
        assert(p);
        free_mem(p);

        U64 merged_after = global_kernel_state.alloc_table.stats.merged_counter[STATS_IDX_WILD];
        assert(merged_after == merged_before + 1);

        I32 defrag = defragment_mem(0);
        assert(defrag == 0);
    });

    // Scenario 3: large number of small blocks coalescing into a tree block (> 2048)
    run_test([]{
        constexpr int kBlocks = 128; // enough to exceed 2048 total
        for (int i = 0; i < kBlocks; ++i) {
            Void* p = try_alloc_mem(32);
            assert(p);
            free_mem(p);
        }
        U64 free_before_tree = global_kernel_state.alloc_table.stats.free_counter[STATS_IDX_TREE];
        I32 defrag = defragment_mem(0);
        assert(defrag >= 1);
        U64 free_after_tree = global_kernel_state.alloc_table.stats.free_counter[STATS_IDX_TREE];
        assert(free_after_tree >= free_before_tree + 1);
        // Expect a non-null tree root
        assert(global_kernel_state.alloc_table.root_free_block != nullptr);
    });

    // Scenario 4: large number of small blocks coalescing into wild block (reach end and merge into wild)
    run_test([]{
        // Allocate and free a sequence large enough to span to wild neighbor on the right
        constexpr int kBlocks = 64;
        for (int i = 0; i < kBlocks; ++i) {
            Void* p = try_alloc_mem(64);
            assert(p);
            free_mem(p);
        }
        U64 merged_wild_before = global_kernel_state.alloc_table.stats.merged_counter[STATS_IDX_WILD];
        I32 defrag = defragment_mem(0);
        // Defragmentation should perform at least one merge chain
        assert(defrag >= 1);
        U64 merged_wild_after = global_kernel_state.alloc_table.stats.merged_counter[STATS_IDX_WILD];
        assert(merged_wild_after >= merged_wild_before + 1);
        // The final block can be wild; ensure pointer is valid
        assert(global_kernel_state.alloc_table.wild_block != nullptr);
        assert(global_kernel_state.alloc_table.wild_block->this_desc.state == (U32)AllocBlockState::WILD_BLOCK);
    });

    // Scenario 5: stats consistency across defragmentation (no change in free_mem_size)
    run_test([]{
        U64 free_mem_before = global_kernel_state.alloc_table.free_mem_size;
        for (int i = 0; i < 16; ++i) {
            Void* p = try_alloc_mem(128);
            assert(p);
            free_mem(p);
        }
        I32 defrag = defragment_mem(0);
        (void)defrag;
        U64 free_mem_after = global_kernel_state.alloc_table.free_mem_size;
        assert(free_mem_after == free_mem_before);
    });

    return 0;
}
