#pragma once

#include "dap_lifecycle.hpp"
#include "dap_basic_inl.hpp"

static inline struct dap_initialize_request* dap_init_initialize_request(void* mem, AkU64 tag_id) noexcept {
    auto* m = (struct dap_initialize_request*)mem;
    dap_msg_hdr_init(&m->hdr, tag_id);
    return m;
}
static inline struct dap_initialize_response* dap_init_initialize_response(void* mem, AkU64 tag_id) noexcept {
    auto* m = (struct dap_initialize_response*)mem;
    dap_msg_hdr_init(&m->hdr, tag_id);
    return m;
}
static inline struct dap_initialize_error_result* dap_init_initialize_error_result(void* mem, AkU64 tag_id) noexcept {
    auto* m = (struct dap_initialize_error_result*)mem;
    dap_msg_hdr_init(&m->hdr, tag_id);
    return m;
}

static inline struct dap_initialized_event* dap_init_initialized_event(void* mem, AkU64 tag_id) noexcept {
    auto* m = (struct dap_initialized_event*)mem;
    dap_msg_hdr_init(&m->hdr, tag_id);
    return m;
}

static inline struct dap_cancel_request* dap_init_cancel_request(void* mem, AkU64 tag_id) noexcept {
    auto* m = (struct dap_cancel_request*)mem;
    dap_msg_hdr_init(&m->hdr, tag_id);
    return m;
}

static inline struct dap_disconnect_request* dap_init_disconnect_request(void* mem, AkU64 tag_id) noexcept {
    auto* m = (struct dap_disconnect_request*)mem;
    dap_msg_hdr_init(&m->hdr, tag_id);
    return m;
}
static inline struct dap_disconnect_response* dap_init_disconnect_response(void* mem, AkU64 tag_id) noexcept {
    auto* m = (struct dap_disconnect_response*)mem;
    dap_msg_hdr_init(&m->hdr, tag_id);
    return m;
}

static inline struct dap_configuration_done_request* dap_init_configuration_done_request(void* mem, AkU64 tag_id) noexcept {
    auto* m = (struct dap_configuration_done_request*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m;
}
static inline struct dap_configuration_done_response* dap_init_configuration_done_response(void* mem, AkU64 tag_id) noexcept {
    auto* m = (struct dap_configuration_done_response*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m;
}

static inline struct dap_launch_request* dap_init_launch_request(void* mem, AkU64 tag_id) noexcept {
    auto* m = (struct dap_launch_request*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m;
}
static inline struct dap_launch_response* dap_init_launch_response(void* mem, AkU64 tag_id) noexcept {
    auto* m = (struct dap_launch_response*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m;
}

static inline struct dap_attach_request* dap_init_attach_request(void* mem, AkU64 tag_id) noexcept {
    auto* m = (struct dap_attach_request*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m;
}
static inline struct dap_attach_response* dap_init_attach_response(void* mem, AkU64 tag_id) noexcept {
    auto* m = (struct dap_attach_response*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m;
}

static inline struct dap_restart_request* dap_init_restart_request(void* mem, AkU64 tag_id) noexcept {
    auto* m = (struct dap_restart_request*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m;
}
static inline struct dap_restart_response* dap_init_restart_response(void* mem, AkU64 tag_id) noexcept {
    auto* m = (struct dap_restart_response*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m;
}

static inline struct dap_terminate_request* dap_init_terminate_request(void* mem, AkU64 tag_id) noexcept {
    auto* m = (struct dap_terminate_request*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m;
}
static inline struct dap_terminate_response* dap_init_terminate_response(void* mem, AkU64 tag_id) noexcept {
    auto* m = (struct dap_terminate_response*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m;
}

static inline struct dap_run_in_terminal_request* dap_init_run_in_terminal_request(void* mem, AkU64 tag_id) noexcept {
    auto* m = (struct dap_run_in_terminal_request*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m;
}
static inline struct dap_run_in_terminal_response* dap_init_run_in_terminal_response(void* mem, AkU64 tag_id) noexcept {
    auto* m = (struct dap_run_in_terminal_response*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m;
}


