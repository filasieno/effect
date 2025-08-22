#include "ak/base/base_api.hpp"
#include <time.h>

// Expect ad linux 64-bit time
static_assert(sizeof(timespec) == 16, "timespec is not 16 bytes");

namespace ak {
    U64 query_timer_ns() noexcept {
        timespec ts;
        ::clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
        return ts.tv_sec * 1e9 + ts.tv_nsec;
    }
}
