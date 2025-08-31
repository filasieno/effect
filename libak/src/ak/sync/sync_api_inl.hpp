#include "ak/sync/sync_api.hpp"

// Public inline API implementation
// --------------------------------

namespace ak {
    
    inline AkVoid init_event(Event* event) {  
        init_AkDLink(&event->wait_list);
    }
    
    inline op::WaitEvent wait(Event* event) {
        AK_ASSERT(event != nullptr);
        return op::WaitEvent{event};
    }

}