
#include "ak/runtime/runtime_api.hpp" // IWYU pragma: keep

#include <cstdlib>

namespace ak {

    Void CThread::Context::unhandled_exception() noexcept {
        std::abort(); /* unreachable */
    }

    BootCThread BootCThread::Context::get_return_object_on_allocation_failure() noexcept {
        std::abort(); /* unreachable */
    }
}


