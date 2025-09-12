#pragma once

#include "dap_threads.hpp"
#include "dap_basic_inl.hpp"

static inline struct dap_threads_request* dap_init_threads_request(void* mem, AkU64 tag_id) noexcept {
    auto* m = (struct dap_threads_request*)mem;
    dap_msg_hdr_init(&m->hdr, tag_id);
    return m;
}
static inline struct dap_threads_response* dap_init_threads_response(void* mem, AkU64 tag_id) noexcept {
    auto* m = (struct dap_threads_response*)mem;
    dap_msg_hdr_init(&m->hdr, tag_id);
    return m;
}

static inline struct dap_terminate_threads_request* dap_init_terminate_threads_request(void* mem, AkU64 tag_id) noexcept {
    auto* m = (struct dap_terminate_threads_request*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m;
}
static inline struct dap_terminate_threads_response* dap_init_terminate_threads_response(void* mem, AkU64 tag_id) noexcept {
    auto* m = (struct dap_terminate_threads_response*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m;
}


