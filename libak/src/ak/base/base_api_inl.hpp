#pragma once

#include "ak/base/base_api.hpp"
#include <cstdio>
#include <format>
#include <print>
#include <source_location>
#include <string_view>
#include <tuple>

inline void ak_dlink_init(ak_dlink* link) noexcept {
    AK_ASSERT(link != nullptr);
    link->next = link;
    link->prev = link;
}

inline bool ak_dlink_is_detached(const ak_dlink* link) noexcept {
    AK_ASSERT(link != nullptr);
    AK_ASSERT(link->next != nullptr);
    AK_ASSERT(link->prev != nullptr);
    return link->next == link && link->prev == link;
}

inline void ak_dlink_detach(ak_dlink* link) noexcept {
    AK_ASSERT(link != nullptr);
    AK_ASSERT(link->next != nullptr);
    AK_ASSERT(link->prev != nullptr);
    if (ak_dlink_is_detached(link)) return;
    link->next->prev = link->prev;
    link->prev->next = link->next;
    link->next = link;
    link->prev = link;
}

inline void ak_dlink_clear(ak_dlink* link) noexcept {
    AK_ASSERT(link != nullptr);
    link->next = nullptr;
    link->prev = nullptr;
}

inline void ak_dlink_enqueue(ak_dlink* queue, ak_dlink* link) noexcept {
    AK_ASSERT(queue != nullptr);
    AK_ASSERT(link != nullptr);
    AK_ASSERT(queue->next != nullptr);
    AK_ASSERT(queue->prev != nullptr);
    link->next = queue->next;
    link->prev = queue;
    link->next->prev = link;
    queue->next = link;
}

inline ak_dlink* ak_dlink_dequeue(ak_dlink* queue) noexcept {
    AK_ASSERT(queue != nullptr);
    AK_ASSERT(queue->next != nullptr);
    AK_ASSERT(queue->prev != nullptr);
    if (ak_dlink_is_detached(queue)) return nullptr;
    ak_dlink* target = queue->prev;
    ak_dlink_detach(target);
    return target;
}

inline void ak_dlink_insert_prev(ak_dlink* queue, ak_dlink* link) noexcept {
    AK_ASSERT(queue != nullptr);
    AK_ASSERT(link != nullptr);
    AK_ASSERT(queue->next != nullptr);
    AK_ASSERT(queue->prev != nullptr);
    link->next = queue;
    link->prev = queue->prev;
    link->next->prev = link;
    link->prev->next = link;
}

inline void ak_dlink_insert_next(ak_dlink* queue, ak_dlink* link) noexcept {
    AK_ASSERT(queue != nullptr);
    AK_ASSERT(link != nullptr);
    AK_ASSERT(queue->next != nullptr);
    AK_ASSERT(queue->prev != nullptr);
    link->next = queue->next;
    link->prev = queue;
    link->next->prev = link;
    queue->next = link;
}

inline void ak_dlink_push(ak_dlink* stack, ak_dlink* link) noexcept { 
    ak_dlink_insert_next(stack, link); 
}

inline ak_dlink* ak_dlink_pop(ak_dlink* stack) noexcept {
    AK_ASSERT(stack != nullptr);
    AK_ASSERT(stack->next != nullptr);
    AK_ASSERT(stack->prev != nullptr);
    AK_ASSERT(!ak_dlink_is_detached(stack));
    ak_dlink* target = stack->next;
    ak_dlink_detach(target);
    return target;
}

template <typename... Args>
inline void ak_ensure(bool condition, const char* expression_text, const std::source_location loc, const std::string_view fmt, Args&&... args) noexcept 
{
    constexpr const char* RESET  = "\033[0m";
    constexpr const char* RED    = "\033[1;31m";
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

