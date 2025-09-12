#pragma once

#include "dap_basic.hpp"

static inline void dap_msg_hdr_init(struct dap_msg_hdr* hdr, AkU64 tag_id) noexcept {
    hdr->timestamp_nanos = ak_query_timer_ns();
    hdr->refcount.store(1, std::memory_order_relaxed);
    hdr->tag_id = tag_id;
}


