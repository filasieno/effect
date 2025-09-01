#pragma once

#include "ak/storage/storage_api.hpp"

inline const AkChar* ak_to_string(AkBufferPool p) noexcept {
    switch (p) {
        case AkBufferPool::INVALID: return "Invalid";
        case AkBufferPool::DEFAULT: return "Default";
        case AkBufferPool::RECYCLE: return "Recycle";
        case AkBufferPool::KEEP:    return "Keep";
    }
    std::abort();
}
