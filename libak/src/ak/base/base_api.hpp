#pragma once

#include <liburing.h>
#include <string_view>  
#include <source_location>

// Macros
#define AK_PACKED_ATTR          __attribute__((packed))
#define AK_OFFSET(TYPE, MEMBER) ((AkSize)((AkU64)&(((TYPE*)0)->MEMBER)))
#define AK_UNLIKELY(x)          __builtin_expect(!!(x), 0)

#define AK_ASSERT(cond, ...)         ::ak::priv::ak_ensure((cond), #cond, std::source_location::current(), ##__VA_ARGS__)
#define AK_ASSERT_AT(loc, cond, ...) ::ak::priv::ak_ensure((cond), #cond, loc                            , ##__VA_ARGS__)

using AkVoid    = void;
using AkBool    = bool;
using AkChar    = char;
using AkWChar   = wchar_t;

using AkU64     = unsigned long long;
using AkU32     = unsigned int;
using AkU16     = unsigned short;
using AkU8      = unsigned char;

using AkI64     = signed long long;
using AkI32     = signed int;
using AkI16     = signed short;
using AkI8      = signed char;

using AkSize    = unsigned long long;
using AkISize   = signed long long;
using AkPtrDiff = signed long long;

using AkF32     = float;
using AkF64     = double;

extern AkI32 AK_MAYOR, AK_MINOR, AK_PATCH, AK_BUILD;

// Build/config flags
#ifdef NDEBUG
constexpr AkBool AK_IS_DEBUG_MODE                = false;
#else
constexpr AkBool AK_IS_DEBUG_MODE                = true;
#endif

constexpr AkBool AK_ENABLE_AVX2                  = false;
constexpr AkBool AK_TRACE_DEBUG_CODE             = false;
constexpr AkBool AK_ENABLE_FULL_INVARIANT_CHECKS = true;
constexpr AkU64  AK_CACHE_LINE_SIZE              = 64;

namespace ak::priv {
    
    struct AkDLink { 
        AkDLink* next; 
        AkDLink* prev; 
    };

    ///\brief Assertion backend
    template <typename... Args>
    inline AkVoid ak_ensure(
        AkBool condition,
        const AkChar* expression_text,
        const std::source_location loc = std::source_location::current(),
        const std::string_view fmt = {},
        Args&&... args
    ) noexcept;

} // namespace ak


AkU64 ak_query_timer_ns() noexcept;