#pragma once

#include "ak/api_priv.hpp"

namespace ak { namespace priv {
        
    // Task runtime debug utilities
    // ----------------------------------------------------------------------------------------------------------------

    inline Void DebugTaskCount() noexcept {
        if constexpr (priv::TRACE_DEBUG_CODE) {
            int runningCount = gKernel.currentTaskHdl != TaskHdl() ? 1 : 0;
            std::print("- {} Running\n", runningCount);
            std::print("  {} Ready\n", gKernel.readyCount);
            std::print("  {} Waiting\n", gKernel.waitingCount);
            std::print("  {} IO waiting\n", gKernel.ioWaitingCount);
            std::print("  {} Zombie\n", gKernel.zombieCount);
        }
    }

    // Check Task Invariants
    // ----------------------------------------------------------------------------------------------------------------

    inline Void DoCheckTaskCountInvariant() noexcept {
        int running_count = gKernel.currentTaskHdl != TaskHdl() ? 1 : 0;
        Bool condition = gKernel.taskCount == running_count + gKernel.readyCount + gKernel.waitingCount + gKernel.ioWaitingCount + gKernel.zombieCount;
        if (!condition) {
            DebugTaskCount();
            abort();
        }
    }

    inline Void CheckTaskCountInvariant() noexcept {
        if constexpr (IS_DEBUG_MODE) {
            DoCheckTaskCountInvariant();
        }
    }

    inline Void CheckInvariants() noexcept {
        if constexpr (IS_DEBUG_MODE) {
            // check the Task invariants
            DoCheckTaskCountInvariant();

            // Ensure that the current Task is valid
            // if (gKernel.currentTask.isValid()) std::abort();
        }
    }

    // IO Uring Debug utils
    // ----------------------------------------------------------------------------------------------------------------
    
    inline Void DebugIOURingFeatures(const unsigned int features) {
        std::print("IO uring features:\n");
        if (features & IORING_FEAT_SINGLE_MMAP)     std::print("  SINGLE_MMAP\n");
        if (features & IORING_FEAT_NODROP)          std::print("  NODROP\n");
        if (features & IORING_FEAT_SUBMIT_STABLE)   std::print("  SUBMIT_STABLE\n");
        if (features & IORING_FEAT_RW_CUR_POS)      std::print("  RW_CUR_POS\n");
        if (features & IORING_FEAT_CUR_PERSONALITY) std::print("  CUR_PERSONALITY\n");
        if (features & IORING_FEAT_FAST_POLL)       std::print("  FAST_POLL\n");
        if (features & IORING_FEAT_POLL_32BITS)     std::print("  POLL_32BITS\n");
        if (features & IORING_FEAT_SQPOLL_NONFIXED) std::print("  SQPOLL_NONFIXED\n");
        if (features & IORING_FEAT_EXT_ARG)         std::print("  EXT_ARG\n");
        if (features & IORING_FEAT_NATIVE_WORKERS)  std::print("  NATIVE_WORKERS\n");
    }

    inline Void DebugIOURingSetupFlags(const unsigned int flags) {
        std::print("IO uring flags:\n");
        if (flags & IORING_SETUP_IOPOLL)    std::print("  IOPOLL\n");
        if (flags & IORING_SETUP_SQPOLL)    std::print("  SQPOLL\n");
        if (flags & IORING_SETUP_SQ_AFF)    std::print("  SQ_AFF\n");
        if (flags & IORING_SETUP_CQSIZE)    std::print("  CQSIZE\n");
        if (flags & IORING_SETUP_CLAMP)     std::print("  CLAMP\n");
        if (flags & IORING_SETUP_ATTACH_WQ) std::print("  ATTACH_WQ\n");
    }

    inline Void DebugIOURingParams(const io_uring_params* p) {
        std::print("IO uring parameters:\n");
        
        // Main parameters
        std::print("Main Configuration:\n");
        std::print("  sq_entries: {}\n", p->sq_entries);
        std::print("  cq_entries: {}\n", p->cq_entries);
        std::print("  sq_thread_cpu: {}\n", p->sq_thread_cpu);
        std::print("  sq_thread_idle: {}\n", p->sq_thread_idle);
        std::print("  wq_fd: {}\n", p->wq_fd);

        // Print flags
        DebugIOURingSetupFlags(p->flags);

        // Print features
        DebugIOURingFeatures(p->features);

        // Submission Queue Offsets

        std::print("Submission Queue Offsets:\n");
        std::print("  head: {}\n", p->sq_off.head);
        std::print("  tail: {}\n", p->sq_off.tail);
        std::print("  ring_mask: {}\n", p->sq_off.ring_mask);
        std::print("  ring_entries: {}\n", p->sq_off.ring_entries);
        std::print("  flags: {}\n", p->sq_off.flags);
        std::print("  dropped: {}\n", p->sq_off.dropped);
        std::print("  array: {}\n", p->sq_off.array);

        // Completion Queue Offsets

        std::print("Completion Queue Offsets:\n");
        std::print("  head: {}\n", p->cq_off.head);
        std::print("  tail: {}\n", p->cq_off.tail);
        std::print("  ring_mask: {}\n", p->cq_off.ring_mask);
        std::print("  ring_entries: {}\n", p->cq_off.ring_entries);
        std::print("  overflow: {}\n", p->cq_off.overflow);
        std::print("  cqes: {}\n", p->cq_off.cqes);
        std::print("  flags: {}\n", p->cq_off.flags);
        std::print("\n");
        std::fflush(stdout);
    }

    // Allocator Debug utils
    // ----------------------------------------------------------------------------------------------------------------

    inline Void DebugDumpAllocTable() noexcept {
        AllocTable* at = &gKernel.alloc_table;

        // Basic layout and sizes
        std::print("AllocTable: {}\n", (Void*)at);
        
        std::print("  heapBegin        : {}\n", (Void*)at->heapBegin);
        std::print("  heapEnd          : {}; size: {}\n", (Void*)at->heapEnd, (intptr_t)(at->heapEnd - at->heapBegin));
        std::print("  memBegin         : {}\n", (Void*)at->memBegin);
        std::print("  memEnd           : {}; size: {}\n", (Void*)at->memEnd,  (intptr_t)(at->memEnd  - at->memBegin));
        std::print("  memSize          : {}\n", at->memSize);
        std::print("  freeMemSize      : {}\n", at->freeMemSize);
    
        // Sentinels and wild/large tracking (addresses only; do not dereference)
        std::print("  Key Offsets:\n");
        std::print("    Begin sentinel offset: {}\n", (intptr_t)at->beginSentinel      - (intptr_t)at->memBegin);
        std::print("    Wild  block    offset: {}\n", (intptr_t)at->wildBlock          - (intptr_t)at->memBegin);
        std::print("    LB    sentinel offset: {}\n", (intptr_t)at->largeBlockSentinel - (intptr_t)at->memBegin);
        std::print("    End   sentinel offset: {}\n", (intptr_t)at->endSentinel        - (intptr_t)at->memBegin);
    
        // Free list availability mask as a bit array (256 bits)
        std::print("  FreeListbinMask:");
        alignas(32) uint64_t lanesPrint[4] = {0,0,0,0};
        static_assert(sizeof(lanesPrint) == 32, "lanesPrint must be 256 bits");
        std::memcpy(lanesPrint, &at->freeListbinMask, 32);
        for (unsigned i = 0; i < 256; i++) {
            if (i % 64 == 0) std::print("\n    ");
            unsigned lane = i >> 6;
            unsigned bit  = i & 63u;
            std::print("{}", (lanesPrint[lane] >> bit) & 1ull);
        }
            std::print("\n");
    
        // Optional per-bin size accounting
        
        std::print("  FreeListBinsSizes begin\n");
        for (unsigned i = 0; i < 254; ++i) {
            unsigned cc = at->freeListBinsCount[i];
            if (cc == 0) continue;
            std::print("    {:>5} bytes class  : {}\n", (i + 1) * 32, cc);
        }
        std::print("     medium     class  : {}\n", at->freeListBinsCount[254]);
        std::print("     wild       class  : {}\n", at->freeListBinsCount[255]);
        std::print("  FreeListBinsSizes end\n");
        
    
        // Aggregate statistics
        // std::print("maxFreeBlockSize: {}\n", at->maxFreeBlockSize);
        // std::print("totalAllocCount: {}\n", at->totalAllocCount);
        // std::print("totalFreeCount: {}\n", at->totalFreeCount);
        // std::print("totalReallocCount: {}\n", at->totalReallocCount);
        // std::print("totalSplitCount: {}\n", at->totalSplitCount);
        // std::print("totalMergeCount: {}\n", at->totalMergeCount);
        // std::print("totalReuseCount: {}\n", at->totalReuseCount);
                
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

    static inline constexpr const Char* StateColor(AllocState s) {
        switch (s) {
            case AllocState::USED:               
                return DEBUG_ALLOC_COLOR_CYAN;
            case AllocState::FREE:   
            case AllocState::WILD_BLOCK: 
                return DEBUG_ALLOC_COLOR_GREEN;
            case AllocState::BEGIN_SENTINEL:
            case AllocState::LARGE_BLOCK_SENTINEL:
            case AllocState::END_SENTINEL: 
                return DEBUG_ALLOC_COLOR_YELLOW;
            case AllocState::INVALID: 
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

    static inline Void PrintRow(const AllocHeader* h) {
        const AllocTable* at = &gKernel.alloc_table;
        uintptr_t beginAddr = (uintptr_t)at->beginSentinel;
        uintptr_t off = (uintptr_t)h - beginAddr;
        uint64_t  sz  = (uint64_t)h->thisSize.size;
        uint64_t  psz = (uint64_t)h->prevSize.size;
        AllocState st = (AllocState)h->thisSize.state;
        AllocState pst = (AllocState)h->prevSize.state;

        const Char* stateText = ToString(st);
        const Char* previousStateText = ToString(pst);
        const Char* stateColor = StateColor(st);

        std::print("{}│{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        std::print("{} {:<18} ", stateColor, (unsigned long long)off);
        std::print("{}│{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        std::print("{} {:<12} ", stateColor, (unsigned long long)sz);
        std::print("{}│{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        std::print("{} {:<10} ", stateColor, stateText);
        std::print("{}│{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        std::print("{} {:<12} ", stateColor, (unsigned long long)psz);
        std::print("{}│{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        std::print("{} {:<10} ", stateColor, previousStateText);
        std::print("{}│{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        
        Size binIdx = GetAllocSmallBinIndexFromSize(h->thisSize.size);  
        
        // Print FreeListPrev
        if (h->thisSize.state == (U32)AllocState::FREE) {
            utl::DLink* freeListLink = &((FreeAllocHeader*)h)->freeListLink;
            utl::DLink* prev = freeListLink->prev;
            utl::DLink* head = &gKernel.alloc_table.freeListBins[binIdx];
            if (prev == head) {
                std::print("{} {:<18} ", stateColor, "HEAD");
            } else {
                AllocHeader* prevBlock = (AllocHeader*)((Char*)prev - offsetof(FreeAllocHeader, freeListLink));
                Size offset = (Size)((Char*)prevBlock - (Char*)gKernel.alloc_table.beginSentinel);
                std::print("{} {:<18} ", stateColor, offset);
            }
        } else {
            std::print("{} {:<18} ", stateColor, "");
        }

        std::print("{}│{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);

        // Print FreeList Next
        if (h->thisSize.state == (U32)AllocState::FREE) {
            utl::DLink* freeListLink = &((FreeAllocHeader*)h)->freeListLink;
            utl::DLink* next = freeListLink->next;
            utl::DLink* head = &gKernel.alloc_table.freeListBins[binIdx];
            if (next == head) {
                std::print("{} {:<18} ", stateColor, "HEAD");
            } else {
                AllocHeader* nextBlock = (AllocHeader*)((Char*)next - offsetof(FreeAllocHeader, freeListLink));
                Size offset = (Size)((Char*)nextBlock - (Char*)gKernel.alloc_table.beginSentinel);
                std::print("{} {:<18} ", stateColor, offset);
            }
        } else {
            std::print("{} {:<18} ", stateColor, "");
        }

        std::print("{}│{}\n", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
    }

    inline Void DebugPrintAllocBlocks() noexcept 
    {
        using namespace priv;
        AllocTable* at = &gKernel.alloc_table;
        
        PrintTopBorder();
        PrintHeader();
        PrintHeaderSeparator();
        AllocHeader* head = (AllocHeader*) at->beginSentinel;
        AllocHeader* end  = (AllocHeader*) NextAllocHeaderPtr((AllocHeader*)at->endSentinel);
        
        for (; head != end; head = NextAllocHeaderPtr(head)) {
            PrintRow(head);
        }

        PrintBottomBorder();
    } 

}} // namespace ak::priv
