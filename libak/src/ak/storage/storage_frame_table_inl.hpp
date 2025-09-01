#include "ak/storage/storage_frame_table.hpp" // IWYU pragma: keep

namespace ak {
    const char* ak_to_string(AkBufferPool p) noexcept {
        switch (p) {
            case AkBufferPool::INVALID: return "Invalid";
            case AkBufferPool::DEFAULT: return "Default";
            case AkBufferPool::RECYCLE: return "Recycle";
            case AkBufferPool::KEEP:    return "Keep";
        }
        std::abort();
    }
}