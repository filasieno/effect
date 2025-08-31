#include "ak/base/base_api.hpp"
#include <time.h>

// Expect ad linux 64-bit time
static_assert(sizeof(timespec) == 16, "timespec is not 16 bytes");


AkU64 ak_query_timer_ns() noexcept {
    timespec ts;
    ::clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return ts.tv_sec * 1000000000 + ts.tv_nsec;
}

