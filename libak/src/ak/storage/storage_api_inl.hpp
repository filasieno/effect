#pragma once

#include "ak/storage/storage_api.hpp"

inline const char* ak_to_string(enum ak_bufferpool p) noexcept {
    switch (p) {
        case AK_BUFFERPOOL_INVALID: return "Invalid";
        case AK_BUFFERPOOL_DEFAULT: return "Default";
        case AK_BUFFERPOOL_RECYCLE: return "Recycle";
        case AK_BUFFERPOOL_KEEP:    return "Keep";
        default:
        std::abort();
    }
}
