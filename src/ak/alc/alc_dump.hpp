#pragma once

#include "ak/alc/alc_api_priv.hpp" // IWYU pragma: keep
#include "ak/alc/alc_check_invariants.hpp" // IWYU pragma: keep

namespace ak { namespace priv {

    // Allocator Debug utils
    // ----------------------------------------------------------------------------------------------------------------

    inline Void dump_alloc_table() noexcept {
        AllocTable* at = &global_kernel_state.alloc_table;

        // Basic layout and sizes
        std::print("AllocTable: {}\n", (Void*)at);
        
        std::print("  heapBegin        : {}\n", (Void*)at->heap_begin);
        std::print("  heapEnd          : {}; size: {}\n", (Void*)at->heap_end, (intptr_t)(at->heap_end - at->heap_begin));
        std::print("  memBegin         : {}\n", (Void*)at->mem_begin);
        std::print("  memEnd           : {}; size: {}\n", (Void*)at->mem_end,  (intptr_t)(at->mem_end  - at->mem_begin));
        std::print("  memSize          : {}\n", at->mem_size);
        std::print("  freeMemSize      : {}\n", at->free_mem_size);
    
        // Sentinels and wild/large tracking (addresses only; do not dereference)
        std::print("  Key Offsets:\n");
        std::print("    Begin sentinel offset: {}\n", (intptr_t)at->sentinel_begin      - (intptr_t)at->mem_begin);
        std::print("    Wild  block    offset: {}\n", (intptr_t)at->wild_block          - (intptr_t)at->mem_begin);
        std::print("    End   sentinel offset: {}\n", (intptr_t)at->sentinel_end        - (intptr_t)at->mem_begin);
    
        // Free list availability mask (64 bits)
        std::print("  FreeListbinMask:\n    ");
        U64 mask = at->freelist_mask;
        for (unsigned i = 0; i < 64; ++i) {
            std::print("{}", (mask >> i) & 1ull);
        }
        std::print("\n");
    
        // Optional per-bin size accounting
        
        std::print("  FreeListBinsSizes begin\n");
        for (unsigned i = 0; i < 64; ++i) {
            unsigned cc = at->freelist_count[i];
            if (cc == 0) continue;
            std::print("    {:>5} bytes class  : {}\n", (i + 1) * 32, cc);
        }
        std::print("  FreeListBinsSizes end\n");
        std::print("\n");
    }


    constexpr const Char* DEBUG_ALLOC_COLOR_RESET  = "\033[0m";
    constexpr const Char* DEBUG_ALLOC_COLOR_WHITE  = "\033[37m"; 
    constexpr const Char* DEBUG_ALLOC_COLOR_GREEN  = "\033[1;32m"; 
    constexpr const Char* DEBUG_ALLOC_COLOR_YELLOW = "\033[1;33m"; 
    constexpr const Char* DEBUG_ALLOC_COLOR_CYAN   = "\033[36m"; 
    constexpr const Char* DEBUG_ALLOC_COLOR_MAG    = "\033[35m"; 
    constexpr const Char* DEBUG_ALLOC_COLOR_RED    = "\033[1;31m"; 
    constexpr const Char* DEBUG_ALLOC_COLOR_HDR    = "\033[36m"; 

    static inline constexpr const Char* StateColor(AllocBlockState s) {
        switch (s) {
            case AllocBlockState::USED:               
                return DEBUG_ALLOC_COLOR_CYAN;
            case AllocBlockState::FREE:   
            case AllocBlockState::WILD_BLOCK: 
                return DEBUG_ALLOC_COLOR_GREEN;
            case AllocBlockState::BEGIN_SENTINEL:
            case AllocBlockState::LARGE_BLOCK_SENTINEL:
            case AllocBlockState::END_SENTINEL: 
                return DEBUG_ALLOC_COLOR_YELLOW;
            case AllocBlockState::INVALID: 
                return DEBUG_ALLOC_COLOR_RED;
            default: 
                return DEBUG_ALLOC_COLOR_RESET;
        }
    }

    // Fixed column widths (constants) in requested order
    constexpr int DEBUG_COL_W_OFF     = 18; // 0x + 16 hex
    constexpr int DEBUG_COL_W_SIZE    = 12;
    constexpr int DEBUG_COL_W_STATE   = 10;
    constexpr int DEBUG_COL_W_PSIZE   = 12;
    constexpr int DEBUG_COL_W_PSTATE  = 10;
    constexpr int DEBUG_COL_W_FL_PREV = 18;
    constexpr int DEBUG_COL_W_FL_NEXT = 18;

    static inline Void PrintRun(const Char* s, int n, const Char* color = DEBUG_ALLOC_COLOR_WHITE) {
        for (int i = 0; i < n; ++i) std::print("{}{}", color, s);
    }

    static inline Void PrintTopBorder() {
        std::print("{}┌{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_OFF + 2);
        std::print("{}┬{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_SIZE + 2);
        std::print("{}┬{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_STATE + 2);
        std::print("{}┬{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_PSIZE + 2);
        std::print("{}┬{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_PSTATE + 2);
        std::print("{}┬{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_FL_PREV + 2);
        std::print("{}┬{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_FL_NEXT + 2);
        std::print("{}┐{}\n", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
    }

    static inline Void PrintHeaderSeparator() {
        std::print("{}├{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_OFF + 2);
        std::print("{}┼{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_SIZE + 2);
        std::print("{}┼{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_STATE + 2);
        std::print("{}┼{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_PSIZE + 2);
        std::print("{}┼{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_PSTATE + 2);
        std::print("{}┼{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_FL_PREV + 2);
        std::print("{}┼{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_FL_NEXT + 2);
        std::print("{}┤{}\n", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
    }

    static inline Void PrintBottomBorder() {
        std::print("{}└{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_OFF + 2);
        std::print("{}┴{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_SIZE + 2);
        std::print("{}┴{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_STATE + 2);
        std::print("{}┴{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_PSIZE + 2);
        std::print("{}┴{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_PSTATE + 2);
        std::print("{}┴{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_FL_PREV + 2);
        std::print("{}┴{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_FL_NEXT + 2);
        std::print("{}┘{}\n", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
    }

    static inline Void PrintHeader() {
        std::print("{}│{}"       , DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        std::print("{} {:<18} "  , DEBUG_ALLOC_COLOR_HDR,   "Offset");
        std::print("{}│{}"       , DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        std::print("{} {:<12} "  , DEBUG_ALLOC_COLOR_HDR,   "Size");
        std::print("{}│{}"       , DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        std::print("{} {:<10} "  , DEBUG_ALLOC_COLOR_HDR,   "State");
        std::print("{}│{}"       , DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        std::print("{} {:<12} "  , DEBUG_ALLOC_COLOR_HDR,   "PrevSize");
        std::print("{}│{}"       , DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        std::print("{} {:<10} "  , DEBUG_ALLOC_COLOR_HDR,   "PrevState");
        std::print("{}│{}"       , DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        std::print("{} {:<18} "  , DEBUG_ALLOC_COLOR_HDR,   "FreeListPrev");
        std::print("{}│{}"       , DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        std::print("{} {:<18} "  , DEBUG_ALLOC_COLOR_HDR,   "FreeListNext");
        std::print("{}│{}\n"     , DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
    }

    static inline Void PrintRow(const AllocBlockHeader* h) {
        const AllocTable* at = &global_kernel_state.alloc_table;
        uintptr_t begin_addr = (uintptr_t)at->sentinel_begin;
        uintptr_t off = (uintptr_t)h - begin_addr;
        uint64_t  sz  = (uint64_t)h->this_desc.size;
        uint64_t  psz = (uint64_t)h->prev_desc.size;
        AllocBlockState st = (AllocBlockState)h->this_desc.state;
        AllocBlockState pst = (AllocBlockState)h->prev_desc.state;

        const Char* state_text = to_string(st);
        const Char* previous_state_text = to_string(pst);
        const Char* state_color = StateColor(st);

        std::print("{}│{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        std::print("{} {:<18} ", state_color, (unsigned long long)off);
        std::print("{}│{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        std::print("{} {:<12} ", state_color, (unsigned long long)sz);
        std::print("{}│{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        std::print("{} {:<10} ", state_color, state_text);
        std::print("{}│{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        std::print("{} {:<12} ", state_color, (unsigned long long)psz);
        std::print("{}│{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        std::print("{} {:<10} ", state_color, previous_state_text);
        std::print("{}│{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        
        Size bin_idx = get_alloc_freelist_index(h->this_desc.size);

        // Print FreeListPrev (with DLink)
        if (h->this_desc.state == (U32)AllocBlockState::FREE && h->this_desc.size <= 2048) {
            utl::DLink* free_list_link = &((AllocPooledFreeBlockHeader*)h)->freelist_link;
            utl::DLink* prev = free_list_link->prev;
            utl::DLink* head = &global_kernel_state.alloc_table.freelist_head[bin_idx];
            if (prev == head) {
                std::print("{} {:<18} ", state_color, "HEAD");
            } else {
                const Size link_off = AK_OFFSET(AllocPooledFreeBlockHeader, freelist_link);
                AllocBlockHeader* prev_block = (AllocBlockHeader*)((Char*)prev - link_off);
                Size offset = (Size)((Char*)prev_block - (Char*)global_kernel_state.alloc_table.sentinel_begin);
                std::print("{} {:<18} ", state_color, offset);
            }
        } else {
            std::print("{} {:<18} ", state_color, "");
        }

        std::print("{}│{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);

        // Print FreeList Next (with DLink)
        if (h->this_desc.state == (U32)AllocBlockState::FREE && h->this_desc.size <= 2048) {
            utl::DLink* free_list_link = &((AllocPooledFreeBlockHeader*)h)->freelist_link;
            utl::DLink* next = free_list_link->next;
            utl::DLink* head = &global_kernel_state.alloc_table.freelist_head[bin_idx];
            if (next == head) {
                std::print("{} {:<18} ", state_color, "HEAD");
            } else {
                const Size link_off = AK_OFFSET(AllocPooledFreeBlockHeader, freelist_link);
                AllocBlockHeader* next_block = (AllocBlockHeader*)((Char*)next - link_off);
                Size offset = (Size)((Char*)next_block - (Char*)global_kernel_state.alloc_table.sentinel_begin);
                std::print("{} {:<18} ", state_color, offset);
            }
        } else {
            std::print("{} {:<18} ", state_color, "");
        }

        std::print("{}│{}\n", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
    }

    inline Void dump_alloc_block() noexcept 
    {
        using namespace priv;
        AllocTable* at = &global_kernel_state.alloc_table;
        
        PrintTopBorder();
        PrintHeader();
        PrintHeaderSeparator();
        AllocBlockHeader* head = (AllocBlockHeader*) at->sentinel_begin;
        AllocBlockHeader* end  = (AllocBlockHeader*) next((AllocBlockHeader*)at->sentinel_end);
        
        for (; head != end; head = next(head)) {
            PrintRow(head);
        }

        PrintBottomBorder();
    }
    


}} // namespace ak::priv


