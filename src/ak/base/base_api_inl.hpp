#pragma once

#include "ak/base/base_api.hpp"

namespace ak { namespace priv {

    template <typename... Args>
    inline Void ensure(Bool condition,
                       const Char* expression_text,
                       const std::source_location loc,
                       const std::string_view fmt,
                       Args&&... args) noexcept {
        constexpr const Char* RESET  = "\033[0m";
        constexpr const Char* RED    = "\033[1;31m";
        if (AK_UNLIKELY(!condition)) {
            std::print("{}{}:{}: Assertion '{}' failed{}", RED, loc.file_name(), (int)loc.line(), expression_text, RESET);
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

    inline Void init_dlink(DLink* link) noexcept {
        AK_ASSERT(link != nullptr);
        link->next = link;
        link->prev = link;
    }

    inline Bool is_dlink_detached(const DLink* link) noexcept {
        AK_ASSERT(link != nullptr);
        AK_ASSERT(link->next != nullptr);
        AK_ASSERT(link->prev != nullptr);
        return link->next == link && link->prev == link;
    }

    inline Void detach_dlink(DLink* link) noexcept {
        AK_ASSERT(link != nullptr);
        AK_ASSERT(link->next != nullptr);
        AK_ASSERT(link->prev != nullptr);
        if (is_dlink_detached(link)) return;
        link->next->prev = link->prev;
        link->prev->next = link->next;
        link->next = link;
        link->prev = link;
    }

    inline Void clear_dlink(DLink* link) noexcept {
        AK_ASSERT(link != nullptr);
        link->next = nullptr;
        link->prev = nullptr;
    }

    inline Void enqueue_dlink(DLink* queue, DLink* link) noexcept {
        AK_ASSERT(queue != nullptr);
        AK_ASSERT(link != nullptr);
        AK_ASSERT(queue->next != nullptr);
        AK_ASSERT(queue->prev != nullptr);
        link->next = queue->next;
        link->prev = queue;
        link->next->prev = link;
        queue->next = link;
    }

    inline DLink* dequeue_dlink(DLink* queue) noexcept {
        AK_ASSERT(queue != nullptr);
        AK_ASSERT(queue->next != nullptr);
        AK_ASSERT(queue->prev != nullptr);
        if (is_dlink_detached(queue)) return nullptr;
        DLink* target = queue->prev;
        detach_dlink(target);
        return target;
    }

    inline Void insert_prev_dlink(DLink* queue, DLink* link) noexcept {
        AK_ASSERT(queue != nullptr);
        AK_ASSERT(link != nullptr);
        AK_ASSERT(queue->next != nullptr);
        AK_ASSERT(queue->prev != nullptr);
        link->next = queue;
        link->prev = queue->prev;
        link->next->prev = link;
        link->prev->next = link;
    }

    inline Void insert_next_dlink(DLink* queue, DLink* link) noexcept {
        AK_ASSERT(queue != nullptr);
        AK_ASSERT(link != nullptr);
        AK_ASSERT(queue->next != nullptr);
        AK_ASSERT(queue->prev != nullptr);
        link->next = queue->next;
        link->prev = queue;
        link->next->prev = link;
        queue->next = link;
    }

    inline Void push_dlink(DLink* stack, DLink* link) noexcept { insert_next_dlink(stack, link); }
    inline DLink* pop_dlink(DLink* stack) noexcept {
        AK_ASSERT(stack != nullptr);
        AK_ASSERT(stack->next != nullptr);
        AK_ASSERT(stack->prev != nullptr);
        AK_ASSERT(!is_dlink_detached(stack));
        DLink* target = stack->next;
        detach_dlink(target);
        return target;
    }

}} // namespace ak::priv

