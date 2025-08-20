#pragma once

#include <print>
#include <format>
#include <source_location>
#include <string_view>
#include <cstdio>
#include <tuple>
#include "ak/api_priv.hpp"

namespace ak { namespace priv {
    // Assertions and ensure helpers
    // ----------------------------------------------------------------------------------------------------------------
    template <typename... Args>
    inline Void ensure(Bool condition,
                       const Char* expression_text,
                       const std::source_location loc,
                       const std::string_view fmt,
                       Args&&... args) noexcept {
        constexpr const Char* RESET  = "\033[0m";
        constexpr const Char* RED    = "\033[1;31m"; 
        if (AK_UNLIKELY(!condition)) {
            std::print("{}{}:{}: Assertion '{}' failed{}",RED, loc.file_name(), (int)loc.line(), expression_text, RESET);
            if (fmt.size() > 0 && !std::is_constant_evaluated()) {
                std::fputs("; ", stdout);
                if constexpr (sizeof...(Args) > 0) {
                    auto arg_tuple = std::forward_as_tuple(std::forward<Args>(args)...);
                    std::apply([&](auto&... refs){
                        auto fmt_args = std::make_format_args(refs...);
                        std::vprint_nonunicode(stdout, fmt, fmt_args);
                    }, arg_tuple);
                } else {
                    std::fwrite(fmt.data(), 1, fmt.size(), stdout);
                }
            }
            std::fputc('\n', stdout);
            std::fflush(stdout);
            std::abort();
        }
    }

        
    // Task runtime debug utilities
    // ----------------------------------------------------------------------------------------------------------------

    inline Void dump_task_count() noexcept {
        if constexpr (priv::TRACE_DEBUG_CODE) {
            int running_count = global_kernel_state.current_cthread != CThread::Hdl() ? 1 : 0;
            std::print("- {} Running\n", running_count);
            std::print("  {} Ready\n", global_kernel_state.ready_cthread_count);
            std::print("  {} Waiting\n", global_kernel_state.waiting_cthread_count);
            std::print("  {} IO waiting\n", global_kernel_state.iowaiting_cthread_count);
            std::print("  {} Zombie\n", global_kernel_state.zombie_cthread_count);
        }
    }

    // Check Task Invariants
    // ----------------------------------------------------------------------------------------------------------------

    inline Void DoCheckTaskCountInvariant() noexcept {
        int running_count = global_kernel_state.current_cthread != CThread::Hdl() ? 1 : 0;
        Bool condition = global_kernel_state.cthread_count == running_count + global_kernel_state.ready_cthread_count + global_kernel_state.waiting_cthread_count + global_kernel_state.iowaiting_cthread_count + global_kernel_state.zombie_cthread_count;
        if (!condition) {
            dump_task_count();
            abort();
        }
    }

    inline Void CheckTaskCountInvariant() noexcept {
        if constexpr (IS_DEBUG_MODE) {
            DoCheckTaskCountInvariant();
        }
    }

    inline Void check_invariants() noexcept {
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

    inline Void dump_io_uring_params(const io_uring_params* params) {
        std::print("IO uring parameters:\n");
        
        // Main parameters
        std::print("Main Configuration:\n");
        std::print("  sq_entries: {}\n", params->sq_entries);
        std::print("  cq_entries: {}\n", params->cq_entries);
        std::print("  sq_thread_cpu: {}\n", params->sq_thread_cpu);
        std::print("  sq_thread_idle: {}\n", params->sq_thread_idle);
        std::print("  wq_fd: {}\n", params->wq_fd);

        // Print flags
        DebugIOURingSetupFlags(params->flags);

        // Print features
        DebugIOURingFeatures(params->features);

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
 

}} // namespace ak::priv
