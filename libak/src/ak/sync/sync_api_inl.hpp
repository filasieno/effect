#include "ak/sync/sync_api.hpp"

// Public inline API implementation
// --------------------------------

inline void ak_init_event(AkEvent* event) {  
    AK_ASSERT(event != nullptr);
    ak_dlink_init(&event->wait_list);
}

inline AkWaitEventOp ak_wait_event(AkEvent* event) {
    AK_ASSERT(event != nullptr);
    return AkWaitEventOp{event};
}