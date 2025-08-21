#pragma once

#include <liburing.h>
#include <source_location>
#include <string_view>
#include <coroutine>
#include <cstdio>
#include <format>
#include <print>

// Macros
#define AK_PACKED_ATTR __attribute__((packed))
#define AK_OFFSET(TYPE, MEMBER) ((::ak::Size)((::ak::U64)&(((TYPE*)0)->MEMBER)))
#define AK_UNLIKELY(x) __builtin_expect(!!(x), 0)

#define AK_ASSERT(cond, ...)         ::ak::priv::ensure((cond), #cond, std::source_location::current(), ##__VA_ARGS__)
#define AK_ASSERT_AT(loc, cond, ...) ::ak::priv::ensure((cond), #cond, loc                            , ##__VA_ARGS__)

namespace ak {
    // Fundamental types
    using Void   = void;
    using Bool   = bool;
    using Char   = char;
    using WChar  = wchar_t;

    using U64    = unsigned long long;
    using U32    = unsigned long;
    using U16    = unsigned short;
    using U8     = unsigned char;

    using I64    = signed long long;
    using I32    = signed int;
    using I16    = signed short;
    using I8     = signed char;

    using Size   = unsigned long long;
    using ISize  = signed long long;
    using PtrDiff = signed long long;

    using F32    = float;
    using F64    = double;

    namespace priv {
        // Build/config flags
        #ifdef NDEBUG
            constexpr Bool IS_DEBUG_MODE = false;
        #else
            constexpr Bool IS_DEBUG_MODE = true;
        #endif
        constexpr Bool ENABLE_AVX2      = false;
        constexpr Bool TRACE_DEBUG_CODE = false;
        constexpr Bool ENABLE_FULL_INVARIANT_CHECKS = true;

        constexpr U64 CACHE_LINE = 64;

        struct DLink { DLink* next; DLink* prev; };

        // Assertion backend
        template <typename... Args>
        inline Void ensure(Bool condition,
                           const Char* expression_text,
                           const std::source_location loc,
                           const std::string_view fmt = {},
                           Args&&... args) noexcept;
    } // namespace priv

} // namespace ak
