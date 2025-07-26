#pragma once

#include "liburing.h"
#include <print>
#include <coroutine>

inline static void* offset_from(void* base, unsigned int offset) { 
    return (void*)((unsigned char*)base + offset);
}

namespace io_utils {

    void uring_debug_features(const unsigned int features) {
        std::printf("IO uring features:\n");
        if (features & IORING_FEAT_SINGLE_MMAP)     std::printf("  SINGLE_MMAP\n");
        if (features & IORING_FEAT_NODROP)          std::printf("  NODROP\n");
        if (features & IORING_FEAT_SUBMIT_STABLE)   std::printf("  SUBMIT_STABLE\n");
        if (features & IORING_FEAT_RW_CUR_POS)      std::printf("  RW_CUR_POS\n");
        if (features & IORING_FEAT_CUR_PERSONALITY) std::printf("  CUR_PERSONALITY\n");
        if (features & IORING_FEAT_FAST_POLL)       std::printf("  FAST_POLL\n");
        if (features & IORING_FEAT_POLL_32BITS)     std::printf("  POLL_32BITS\n");
        if (features & IORING_FEAT_SQPOLL_NONFIXED) std::printf("  SQPOLL_NONFIXED\n");
        if (features & IORING_FEAT_EXT_ARG)         std::printf("  EXT_ARG\n");
        if (features & IORING_FEAT_NATIVE_WORKERS)  std::printf("  NATIVE_WORKERS\n");
    }

    void uring_debug_setup_flags(const unsigned int flags) {
        std::printf("IO uring flags:\n");
        if (flags & IORING_SETUP_IOPOLL)    std::printf("  IOPOLL\n");
        if (flags & IORING_SETUP_SQPOLL)    std::printf("  SQPOLL\n");
        if (flags & IORING_SETUP_SQ_AFF)    std::printf("  SQ_AFF\n");
        if (flags & IORING_SETUP_CQSIZE)    std::printf("  CQSIZE\n");
        if (flags & IORING_SETUP_CLAMP)     std::printf("  CLAMP\n");
        if (flags & IORING_SETUP_ATTACH_WQ) std::printf("  ATTACH_WQ\n");
    }


    void uring_debug_params(const struct io_uring_params& p) {
        std::printf("IO uring parameters:\n");
        
        // Main parameters
        std::printf("Main Configuration:\n");
        std::printf("  sq_entries: %u\n", p.sq_entries);
        std::printf("  cq_entries: %u\n", p.cq_entries);
        std::printf("  sq_thread_cpu: %u\n", p.sq_thread_cpu);
        std::printf("  sq_thread_idle: %u\n", p.sq_thread_idle);
        std::printf("  wq_fd: %u\n", p.wq_fd);

        // Print flags
        uring_debug_setup_flags(p.flags);

        // Print features
        uring_debug_features(p.features);

        // Submission Queue Offsets

        std::printf("Submission Queue Offsets:\n");
        std::printf("  head: %u\n", p.sq_off.head);
        std::printf("  tail: %u\n", p.sq_off.tail);
        std::printf("  ring_mask: %u\n", p.sq_off.ring_mask);
        std::printf("  ring_entries: %u\n", p.sq_off.ring_entries);
        std::printf("  flags: %u\n", p.sq_off.flags);
        std::printf("  dropped: %u\n", p.sq_off.dropped);
        std::printf("  array: %u\n", p.sq_off.array);

        // Completion Queue Offsets

        std::printf("Completion Queue Offsets:\n");
        std::printf("  head: %u\n", p.cq_off.head);
        std::printf("  tail: %u\n", p.cq_off.tail);
        std::printf("  ring_mask: %u\n", p.cq_off.ring_mask);
        std::printf("  ring_entries: %u\n", p.cq_off.ring_entries);
        std::printf("  overflow: %u\n", p.cq_off.overflow);
        std::printf("  cqes: %u\n", p.cq_off.cqes);
        std::printf("  flags: %u\n", p.cq_off.flags);
        std::printf("\n");
    }
}

