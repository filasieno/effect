#pragma once

#include "ak/base/base_api.hpp"
#include <cstdio>
#include <format>
#include <print>
#include <source_location>
#include <string_view>
#include <tuple>


namespace ak { namespace priv {

    template <typename... Args>
    inline AkVoid ak_ensure(AkBool condition,
                       const AkChar* expression_text,
                       const std::source_location loc,
                       const std::string_view fmt,
                       Args&&... args) noexcept {
        constexpr const AkChar* RESET  = "\033[0m";
        constexpr const AkChar* RED    = "\033[1;31m";
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

    inline AkVoid init_AkDLink(AkDLink* link) noexcept {
        AK_ASSERT(link != nullptr);
        link->next = link;
        link->prev = link;
    }

    inline AkBool is_AkDLink_detached(const AkDLink* link) noexcept {
        AK_ASSERT(link != nullptr);
        AK_ASSERT(link->next != nullptr);
        AK_ASSERT(link->prev != nullptr);
        return link->next == link && link->prev == link;
    }

    inline AkVoid detach_AkDLink(AkDLink* link) noexcept {
        AK_ASSERT(link != nullptr);
        AK_ASSERT(link->next != nullptr);
        AK_ASSERT(link->prev != nullptr);
        if (is_AkDLink_detached(link)) return;
        link->next->prev = link->prev;
        link->prev->next = link->next;
        link->next = link;
        link->prev = link;
    }

    inline AkVoid clear_AkDLink(AkDLink* link) noexcept {
        AK_ASSERT(link != nullptr);
        link->next = nullptr;
        link->prev = nullptr;
    }

    inline AkVoid enqueue_AkDLink(AkDLink* queue, AkDLink* link) noexcept {
        AK_ASSERT(queue != nullptr);
        AK_ASSERT(link != nullptr);
        AK_ASSERT(queue->next != nullptr);
        AK_ASSERT(queue->prev != nullptr);
        link->next = queue->next;
        link->prev = queue;
        link->next->prev = link;
        queue->next = link;
    }

    inline AkDLink* dequeue_AkDLink(AkDLink* queue) noexcept {
        AK_ASSERT(queue != nullptr);
        AK_ASSERT(queue->next != nullptr);
        AK_ASSERT(queue->prev != nullptr);
        if (is_AkDLink_detached(queue)) return nullptr;
        AkDLink* target = queue->prev;
        detach_AkDLink(target);
        return target;
    }

    inline AkVoid insert_prev_AkDLink(AkDLink* queue, AkDLink* link) noexcept {
        AK_ASSERT(queue != nullptr);
        AK_ASSERT(link != nullptr);
        AK_ASSERT(queue->next != nullptr);
        AK_ASSERT(queue->prev != nullptr);
        link->next = queue;
        link->prev = queue->prev;
        link->next->prev = link;
        link->prev->next = link;
    }

    inline AkVoid insert_next_AkDLink(AkDLink* queue, AkDLink* link) noexcept {
        AK_ASSERT(queue != nullptr);
        AK_ASSERT(link != nullptr);
        AK_ASSERT(queue->next != nullptr);
        AK_ASSERT(queue->prev != nullptr);
        link->next = queue->next;
        link->prev = queue;
        link->next->prev = link;
        queue->next = link;
    }

    inline AkVoid push_AkDLink(AkDLink* stack, AkDLink* link) noexcept { 
        insert_next_AkDLink(stack, link); 
    }

    inline AkDLink* pop_AkDLink(AkDLink* stack) noexcept {
        AK_ASSERT(stack != nullptr);
        AK_ASSERT(stack->next != nullptr);
        AK_ASSERT(stack->prev != nullptr);
        AK_ASSERT(!is_AkDLink_detached(stack));
        AkDLink* target = stack->next;
        detach_AkDLink(target);
        return target;
    }

}} // namespace ak::priv

