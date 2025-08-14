#pragma once

#include "ak/api_priv.hpp"

namespace ak { namespace priv {
        
    // Task runtime debug utilities
    // ----------------------------------------------------------------------------------------------------------------

    inline void DebugTaskCount() noexcept {
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

    inline void DoCheckTaskCountInvariant() noexcept {
        int running_count = gKernel.currentTaskHdl != TaskHdl() ? 1 : 0;
        bool condition = gKernel.taskCount == running_count + gKernel.readyCount + gKernel.waitingCount + gKernel.ioWaitingCount + gKernel.zombieCount;
        if (!condition) {
            DebugTaskCount();
            abort();
        }
    }

    inline void CheckTaskCountInvariant() noexcept {
        if constexpr (IS_DEBUG_MODE) {
            DoCheckTaskCountInvariant();
        }
    }

    inline void CheckInvariants() noexcept {
        if constexpr (IS_DEBUG_MODE) {
            // check the Task invariants
            DoCheckTaskCountInvariant();

            // Ensure that the current Task is valid
            // if (gKernel.currentTask.isValid()) std::abort();
        }
    }

    // IO Uring Debug utils
    // ----------------------------------------------------------------------------------------------------------------
    
    inline void DebugIOURingFeatures(const unsigned int features) {
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

    inline void DebugIOURingSetupFlags(const unsigned int flags) {
        std::print("IO uring flags:\n");
        if (flags & IORING_SETUP_IOPOLL)    std::print("  IOPOLL\n");
        if (flags & IORING_SETUP_SQPOLL)    std::print("  SQPOLL\n");
        if (flags & IORING_SETUP_SQ_AFF)    std::print("  SQ_AFF\n");
        if (flags & IORING_SETUP_CQSIZE)    std::print("  CQSIZE\n");
        if (flags & IORING_SETUP_CLAMP)     std::print("  CLAMP\n");
        if (flags & IORING_SETUP_ATTACH_WQ) std::print("  ATTACH_WQ\n");
    }

    inline void DebugIOURingParams(const io_uring_params* p) {
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

    inline void DebugDumpAllocTable(AllocTable* at) noexcept {

        // Basic layout and sizes
        std::print("AllocTable: {}\n", (void*)at);
        
        std::print("  heapBegin        : {}\n", (void*)at->heapBegin);
        std::print("  heapEnd          : {}; size: {}\n", (void*)at->heapEnd, (intptr_t)(at->heapEnd - at->heapBegin));
        std::print("  memBegin         : {}\n", (void*)at->memBegin);
        std::print("  memEnd           : {}; size: {}\n", (void*)at->memEnd, (intptr_t)(at->memEnd - at->memBegin));
        std::print("  memSize          : {}\n", at->memSize);
        std::print("  usedMemSize      : {}\n", at->usedMemSize);
        std::print("  freeMemSize      : {}\n", at->freeMemSize);
    
        // Sentinels and wild/large tracking (addresses only; do not dereference)
        std::print("  Key Offsets:\n");
        std::print("    Begin sentinel offset: {}\n", (intptr_t)at->beginSentinel - (intptr_t)at->memBegin);
        std::print("    Wild  block    offset: {}\n", (intptr_t)at->wildBlock - (intptr_t)at->memBegin);
        std::print("    LB    sentinel offset: {}\n", (intptr_t)at->largeBlockSentinel - (intptr_t)at->memBegin);
        std::print("    End   sentinel offset: {}\n", (intptr_t)at->endSentinel - (intptr_t)at->memBegin);
    
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
            std::print("    {:>3} bytes class  : {}\n", i * 32, cc);
        }
        std::print("    medium class (254) : {}\n", at->freeListBinsCount[254]);
        std::print("    wild class   (255) : {}\n", at->freeListBinsCount[255]);
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

}} // namespace ak::priv
