#include "ak/base/base_api.hpp" // IWYU pragma: keep

// Public inline API implementation
// --------------------------------

namespace ak {
    
    inline Void init(Event* event) {  
        init_dlink(&event->wait_list);
    }
    
    inline op::WaitEvent wait(Event* event) {
        AK_ASSERT(event != nullptr);
        return op::WaitEvent{event};
    }

}