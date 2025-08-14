#pragma once

#include "ak/core.hpp"
#include <cstring>

namespace ak {

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
    std::print("Main Configuration:\n");
    std::print("  sq_entries: {}\n", p->sq_entries);
    std::print("  cq_entries: {}\n", p->cq_entries);
    std::print("  sq_thread_cpu: {}\n", p->sq_thread_cpu);
    std::print("  sq_thread_idle: {}\n", p->sq_thread_idle);
    std::print("  wq_fd: {}\n", p->wq_fd);
    DebugIOURingSetupFlags(p->flags);
    DebugIOURingFeatures(p->features);
    std::print("Submission Queue Offsets:\n");
    std::print("  head: {}\n", p->sq_off.head);
    std::print("  tail: {}\n", p->sq_off.tail);
    std::print("  ring_mask: {}\n", p->sq_off.ring_mask);
    std::print("  ring_entries: {}\n", p->sq_off.ring_entries);
    std::print("  flags: {}\n", p->sq_off.flags);
    std::print("  dropped: {}\n", p->sq_off.dropped);
    std::print("  array: {}\n", p->sq_off.array);
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

inline void DebugDumpAllocTable() noexcept {
    using namespace internal;
    AllocTable* at = (AllocTable*)&gKernel.allocTable;
    std::print("AllocTable: {}\n", (void*)at);
    std::print("  heapBegin        : {}\n", (void*)at->heapBegin);
    std::print("  heapEnd          : {}; size: {}\n", (void*)at->heapEnd, (intptr_t)(at->heapEnd - at->heapBegin));
    std::print("  memBegin         : {}\n", (void*)at->memBegin);
    std::print("  memEnd           : {}; size: {}\n", (void*)at->memEnd, (intptr_t)(at->memEnd - at->memBegin));
    std::print("  memSize          : {}\n", at->memSize);
    std::print("  usedMemSize      : {}\n", at->usedMemSize);
    std::print("  freeMemSize      : {}\n", at->freeMemSize);
    std::print("  Key Offsets:\n");
    std::print("    Begin sentinel offset: {}\n", (intptr_t)at->beginSentinel - (intptr_t)at->memBegin);
    std::print("    Wild  block    offset: {}\n", (intptr_t)at->wildBlock - (intptr_t)at->memBegin);
    std::print("    LB    sentinel offset: {}\n", (intptr_t)at->largeBlockSentinel - (intptr_t)at->memBegin);
    std::print("    End   sentinel offset: {}\n", (intptr_t)at->endSentinel - (intptr_t)at->memBegin);
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
    std::print("  FreeListBinsSizes begin\n");
    for (unsigned i = 0; i < 254; ++i) {
        unsigned cc = at->freeListBinsCount[i];
        if (cc == 0) continue;
        std::print("    {:>3} bytes class  : {}\n", i * 32, cc);
    }
    std::print("    medium class (254) : {}\n", at->freeListBinsCount[254]);
    std::print("    wild class   (255) : {}\n", at->freeListBinsCount[255]);
    std::print("  FreeListBinsSizes end\n\n");
}

} // namespace ak



