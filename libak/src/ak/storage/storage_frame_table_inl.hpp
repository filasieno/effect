#include "ak/storage/storage_frame_table.hpp" // IWYU pragma: keep

namespace ak {
    const char* to_string(BufferPool p) noexcept {
        switch (p) {
            case BufferPool::INVALID: return "Invalid";
            case BufferPool::DEFAULT: return "Default";
            case BufferPool::RECYCLE: return "Recycle";
            case BufferPool::KEEP:    return "Keep";
        }
        std::abort();
    }
}