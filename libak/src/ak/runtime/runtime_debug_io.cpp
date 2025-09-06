#include "ak/runtime/runtime.hpp" // IWYU pragma: keep

#include <print>
#include <format>
#include <cstdio>

namespace ak::priv {

    // IO Uring Debug utils
    // ----------------------------------------------------------------------------------------------------------------
    
    void dump_io_uring_features(const unsigned int features) {
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

    void dump_io_uring_setup_flags(const unsigned int flags) {
        std::print("IO uring flags:\n");
        if (flags & IORING_SETUP_IOPOLL)    std::print("  IOPOLL\n");
        if (flags & IORING_SETUP_SQPOLL)    std::print("  SQPOLL\n");
        if (flags & IORING_SETUP_SQ_AFF)    std::print("  SQ_AFF\n");
        if (flags & IORING_SETUP_CQSIZE)    std::print("  CQSIZE\n");
        if (flags & IORING_SETUP_CLAMP)     std::print("  CLAMP\n");
        if (flags & IORING_SETUP_ATTACH_WQ) std::print("  ATTACH_WQ\n");
    }

    void dump_io_uring_params(const io_uring_params* params) {
        std::print("IO uring parameters:\n");
        
        // Main parameters
        std::print("Main Configuration:\n");
        std::print("  sq_entries: {}\n", params->sq_entries);
        std::print("  cq_entries: {}\n", params->cq_entries);
        std::print("  sq_thread_cpu: {}\n", params->sq_thread_cpu);
        std::print("  sq_thread_idle: {}\n", params->sq_thread_idle);
        std::print("  wq_fd: {}\n", params->wq_fd);

        // Print flags
        dump_io_uring_setup_flags(params->flags);

        // Print features
        dump_io_uring_features(params->features);

        // Submission Queue Offsets

        std::print("Submission Queue Offsets:\n");
        std::print("  head: {}\n", params->sq_off.head);
        std::print("  tail: {}\n", params->sq_off.tail);
        std::print("  ring_mask: {}\n", params->sq_off.ring_mask);
        std::print("  ring_entries: {}\n", params->sq_off.ring_entries);
        std::print("  flags: {}\n", params->sq_off.flags);
        std::print("  dropped: {}\n", params->sq_off.dropped);
        std::print("  array: {}\n", params->sq_off.array);

        // Completion Queue Offsets

        std::print("Completion Queue Offsets:\n");
        std::print("  head: {}\n", params->cq_off.head);
        std::print("  tail: {}\n", params->cq_off.tail);
        std::print("  ring_mask: {}\n", params->cq_off.ring_mask);
        std::print("  ring_entries: {}\n", params->cq_off.ring_entries);
        std::print("  overflow: {}\n", params->cq_off.overflow);
        std::print("  cqes: {}\n", params->cq_off.cqes);
        std::print("  flags: {}\n", params->cq_off.flags);
        std::print("\n");
        std::fflush(stdout);
    }

} // namespace ak::priv
