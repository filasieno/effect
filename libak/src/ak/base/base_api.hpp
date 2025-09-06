#pragma once

#include <liburing.h>
#include <string_view>  
#include <source_location>

// Macros
#define AK_UNLIKELY(x)               __builtin_expect(!!(x), 0)
#define AK_MUST_TAIL                 __attribute__((musttail))
#define AK_PACKED_ATTR               __attribute__((packed))
#define AK_OFFSET(TYPE, MEMBER)      ((AkSize)((AkU64)&(((TYPE*)0)->MEMBER)))

#define AK_ASSERT(cond, ...)         ak_ensure((cond), #cond, std::source_location::current(), ##__VA_ARGS__)
#define AK_ASSERT_AT(loc, cond, ...) ak_ensure((cond), #cond, loc                            , ##__VA_ARGS__)

using AkU64     = unsigned long long;
using AkU32     = unsigned int;
using AkU16     = unsigned short;
using AkU8      = unsigned char;

using AkI64     = signed long long;
using AkI16     = signed short;
using AkI8      = signed char;

using AkSize    = unsigned long long;
using AkISize   = signed long long;
using AkPtrDiff = signed long long;

using AkF32     = float;
using AkF64     = double;

extern int AK_MAYOR, AK_MINOR, AK_PATCH, AK_BUILD;

// Build/config flags
#ifdef NDEBUG
constexpr bool AK_IS_DEBUG_MODE                = false;
#else
constexpr bool AK_IS_DEBUG_MODE                = true;
#endif

constexpr bool AK_ENABLE_AVX2                  = false;
constexpr bool AK_TRACE_DEBUG_CODE             = false;
constexpr bool AK_ENABLE_FULL_INVARIANT_CHECKS = true;
constexpr AkU64  AK_CACHE_LINE_SIZE            = 64;

struct ak_dlink { 
    struct ak_dlink* next; 
    struct ak_dlink* prev; 
};

void             ak_dlink_init(struct ak_dlink* link) noexcept;
bool             ak_dlink_is_detached(const struct ak_dlink* link) noexcept;
void             ak_dlink_detach(struct ak_dlink* link) noexcept;
void             ak_dlink_clear(struct ak_dlink* link) noexcept;
void             ak_dlink_enqueue(struct ak_dlink* queue, struct ak_dlink* link) noexcept;
struct ak_dlink* ak_dlink_dequeue(struct ak_dlink* queue) noexcept;
void             ak_dlink_insert_prev(struct ak_dlink* list, struct ak_dlink* link) noexcept;
void             ak_dlink_insert_next(struct ak_dlink* list, struct ak_dlink* link) noexcept;
void             ak_dlink_push(struct ak_dlink* stack, struct ak_dlink* link) noexcept;
struct ak_dlink* ak_dlink_pop(struct ak_dlink* stack) noexcept;

///\brief Assertion backend
template <typename... Args>
inline void ak_ensure(
    bool condition,
    const char* expression_text,
    const std::source_location loc = std::source_location::current(),
    const std::string_view fmt = {},
    Args&&... args
) noexcept;

AkU64 ak_query_timer_ns() noexcept;