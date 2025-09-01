#include "ak/alloc/alloc.hpp" // IWYU pragma: keep

constexpr const AkChar* DEBUG_ALLOC_COLOR_RESET  = "\033[0m";
constexpr const AkChar* DEBUG_ALLOC_COLOR_WHITE  = "\033[37m"; 
constexpr const AkChar* DEBUG_ALLOC_COLOR_GREEN  = "\033[1;32m"; 
constexpr const AkChar* DEBUG_ALLOC_COLOR_YELLOW = "\033[1;33m"; 
constexpr const AkChar* DEBUG_ALLOC_COLOR_CYAN   = "\033[36m";  
constexpr const AkChar* DEBUG_ALLOC_COLOR_RED    = "\033[1;31m"; 
constexpr const AkChar* DEBUG_ALLOC_COLOR_HDR    = "\033[36m"; 

static inline constexpr const AkChar* alloc_get_color_by_block_state(AkAllocBlockState s) {
    switch (s) {
        case AkAllocBlockState::USED:               
            return DEBUG_ALLOC_COLOR_CYAN;
        case AkAllocBlockState::FREE:   
        case AkAllocBlockState::WILD_BLOCK: 
            return DEBUG_ALLOC_COLOR_GREEN;
        case AkAllocBlockState::BEGIN_SENTINEL:
        case AkAllocBlockState::LARGE_BLOCK_SENTINEL:
        case AkAllocBlockState::END_SENTINEL: 
            return DEBUG_ALLOC_COLOR_YELLOW;
        case AkAllocBlockState::INVALID: 
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

static inline AkVoid alloc_debug_print_run(const AkChar* s, int n, const AkChar* color = DEBUG_ALLOC_COLOR_WHITE) {
    for (int i = 0; i < n; ++i) std::print("{}{}", color, s);
}

static inline AkVoid alloc_debug_dump_top_border() {
    std::print("{}┌{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
    alloc_debug_print_run("─", DEBUG_COL_W_OFF + 2);
    std::print("{}┬{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
    alloc_debug_print_run("─", DEBUG_COL_W_SIZE + 2);
    std::print("{}┬{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
    alloc_debug_print_run("─", DEBUG_COL_W_STATE + 2);
    std::print("{}┬{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
    alloc_debug_print_run("─", DEBUG_COL_W_PSIZE + 2);
    std::print("{}┬{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
    alloc_debug_print_run("─", DEBUG_COL_W_PSTATE + 2);
    std::print("{}┬{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
    alloc_debug_print_run("─", DEBUG_COL_W_FL_PREV + 2);
    std::print("{}┬{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
    alloc_debug_print_run("─", DEBUG_COL_W_FL_NEXT + 2);
    std::print("{}┐{}\n", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
}

static inline AkVoid alloc_debug_dump_separator() {
    std::print("{}├{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
    alloc_debug_print_run("─", DEBUG_COL_W_OFF + 2);
    std::print("{}┼{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
    alloc_debug_print_run("─", DEBUG_COL_W_SIZE + 2);
    std::print("{}┼{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
    alloc_debug_print_run("─", DEBUG_COL_W_STATE + 2);
    std::print("{}┼{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
    alloc_debug_print_run("─", DEBUG_COL_W_PSIZE + 2);
    std::print("{}┼{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
    alloc_debug_print_run("─", DEBUG_COL_W_PSTATE + 2);
    std::print("{}┼{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
    alloc_debug_print_run("─", DEBUG_COL_W_FL_PREV + 2);
    std::print("{}┼{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
    alloc_debug_print_run("─", DEBUG_COL_W_FL_NEXT + 2);
    std::print("{}┤{}\n", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
}

static inline AkVoid alloc_debug_dump_bottim_border() {
    std::print("{}└{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
    alloc_debug_print_run("─", DEBUG_COL_W_OFF + 2);
    std::print("{}┴{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
    alloc_debug_print_run("─", DEBUG_COL_W_SIZE + 2);
    std::print("{}┴{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
    alloc_debug_print_run("─", DEBUG_COL_W_STATE + 2);
    std::print("{}┴{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
    alloc_debug_print_run("─", DEBUG_COL_W_PSIZE + 2);
    std::print("{}┴{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
    alloc_debug_print_run("─", DEBUG_COL_W_PSTATE + 2);
    std::print("{}┴{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
    alloc_debug_print_run("─", DEBUG_COL_W_FL_PREV + 2);
    std::print("{}┴{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
    alloc_debug_print_run("─", DEBUG_COL_W_FL_NEXT + 2);
    std::print("{}┘{}\n", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
}

static inline AkVoid alloc_debug_dump_header() {
    std::print("{}│{}"       , DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
    std::print("{} {:<18} "  , DEBUG_ALLOC_COLOR_HDR,   "Offset");
    std::print("{}│{}"       , DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
    std::print("{} {:<12} "  , DEBUG_ALLOC_COLOR_HDR,   "AkSize");
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

static inline AkVoid alloc_debug_dump_row(const AkAllocTable* at, const AkAllocBlockHeader* h) {
    
    uintptr_t begin_addr = (uintptr_t)at->sentinel_begin;
    uintptr_t off = (uintptr_t)h - begin_addr;
    uint64_t  sz  = (uint64_t)h->this_desc.size;
    uint64_t  psz = (uint64_t)h->prev_desc.size;
    AkAllocBlockState st = (AkAllocBlockState)h->this_desc.state;
    AkAllocBlockState pst = (AkAllocBlockState)h->prev_desc.state;

    const AkChar* state_text = to_string(st);
    const AkChar* previous_state_text = to_string(pst);
    const AkChar* state_color = alloc_get_color_by_block_state(st);

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
    
    AkSize bin_idx = alloc_freelist_get_index(h->this_desc.size);

    // Print FreeListPrev (with AkDLink)
    if (h->this_desc.state == (AkU32)AkAllocBlockState::FREE && h->this_desc.size <= 2048) {
        const AkDLink* free_list_link = &((AkAllocPooledFreeBlockHeader*)h)->freelist_link;
        const AkDLink* prev = free_list_link->prev;
        const AkDLink* head = &at->freelist_head[bin_idx];
        if (prev == head) {
            std::print("{} {:<18} ", state_color, "HEAD");
        } else {
            const AkSize link_off = AK_OFFSET(AkAllocPooledFreeBlockHeader, freelist_link);
            AkAllocBlockHeader* prev_block = (AkAllocBlockHeader*)((AkChar*)prev - link_off);
            AkSize offset = (AkSize)((AkChar*)prev_block - (AkChar*)at->sentinel_begin);
            std::print("{} {:<18} ", state_color, offset);
        }
    } else {
        std::print("{} {:<18} ", state_color, "");
    }

    std::print("{}│{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);

    // Print FreeList Next (with AkDLink)
    if (h->this_desc.state == (AkU32)AkAllocBlockState::FREE && h->this_desc.size <= 2048) {
        const AkDLink* free_list_link = &((AkAllocPooledFreeBlockHeader*)h)->freelist_link;
        const AkDLink* next = free_list_link->next;
        const AkDLink* head = &at->freelist_head[bin_idx];
        if (next == head) {
            std::print("{} {:<18} ", state_color, "HEAD");
        } else {
            const AkSize link_off = AK_OFFSET(AkAllocPooledFreeBlockHeader, freelist_link);
            AkAllocBlockHeader* next_block = (AkAllocBlockHeader*)((AkChar*)next - link_off);
            AkSize offset = (AkSize)((AkChar*)next_block - (AkChar*)at->sentinel_begin);
            std::print("{} {:<18} ", state_color, offset);
        }
    } else {
        std::print("{} {:<18} ", state_color, "");
    }

    std::print("{}│{}\n", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
}

AkVoid dump_alloc_table(const AkAllocTable* at) noexcept {
    
    // Basic layout and sizes
    std::print("AllocTable: {}\n", (AkVoid*)at);
    
    std::print("  heapBegin        : {}\n", (AkVoid*)at->heap_begin);
    std::print("  heapEnd          : {}; size: {}\n", (AkVoid*)at->heap_end, (intptr_t)(at->heap_end - at->heap_begin));
    std::print("  memBegin         : {}\n", (AkVoid*)at->mem_begin);
    std::print("  memEnd           : {}; size: {}\n", (AkVoid*)at->mem_end,  (intptr_t)(at->mem_end  - at->mem_begin));
    std::print("  memSize          : {}\n", at->mem_size);
    std::print("  freeMemSize      : {}\n", at->free_mem_size);

    // Sentinels and wild/large tracking (addresses only; do not dereference)
    std::print("  Key Offsets:\n");
    std::print("    Begin sentinel offset: {}\n", (intptr_t)at->sentinel_begin      - (intptr_t)at->mem_begin);
    std::print("    Wild  block    offset: {}\n", (intptr_t)at->wild_block          - (intptr_t)at->mem_begin);
    std::print("    End   sentinel offset: {}\n", (intptr_t)at->sentinel_end        - (intptr_t)at->mem_begin);

    // Free list availability mask (64 bits)
    std::print("  FreeListbinMask:\n    ");
    AkU64 mask = at->freelist_mask;
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

AkVoid alloc_debug_dump_alloc_table(const AkAllocTable* at) noexcept 
{
    alloc_debug_dump_top_border();
    alloc_debug_dump_header();
    alloc_debug_dump_separator();
    AkAllocBlockHeader* head = (AkAllocBlockHeader*) at->sentinel_begin;
    AkAllocBlockHeader* end  = (AkAllocBlockHeader*) alloc_block_next((AkAllocBlockHeader*)at->sentinel_end);
    
    for (; head != end; head = alloc_block_next(head)) {
        alloc_debug_dump_row(at, head);
    }

    alloc_debug_dump_bottim_border();
}



