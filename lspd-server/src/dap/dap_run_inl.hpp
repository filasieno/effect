#pragma once

#include "dap_run.hpp"
#include "dap_basic_inl.hpp"

static inline struct dap_continue_request *dap_init_continue_request(void *mem, AkU64 tag_id) noexcept {
    auto *m = (struct dap_continue_request *)mem;
    dap_msg_hdr_init(&m->hdr, tag_id);
    return m;
}
static inline struct dap_continue_response *dap_init_continue_response(void *mem, AkU64 tag_id) noexcept {
    auto *m = (struct dap_continue_response *)mem;
    dap_msg_hdr_init(&m->hdr, tag_id);
    return m;
}
static inline struct dap_next_request *dap_init_next_request(void *mem, AkU64 tag_id) noexcept {
    auto *m = (struct dap_next_request *)mem;
    dap_msg_hdr_init(&m->hdr, tag_id);
    return m;
}
static inline struct dap_next_response *dap_init_next_response(void *mem, AkU64 tag_id) noexcept {
    auto *m = (struct dap_next_response *)mem;
    dap_msg_hdr_init(&m->hdr, tag_id);
    return m;
}
static inline struct dap_step_in_request *dap_init_step_in_request(void *mem, AkU64 tag_id) noexcept {
    auto *m = (struct dap_step_in_request *)mem;
    dap_msg_hdr_init(&m->hdr, tag_id);
    return m;
}
static inline struct dap_step_in_response *dap_init_step_in_response(void *mem, AkU64 tag_id) noexcept {
    auto *m = (struct dap_step_in_response *)mem;
    dap_msg_hdr_init(&m->hdr, tag_id);
    return m;
}
static inline struct dap_step_out_request *dap_init_step_out_request(void *mem, AkU64 tag_id) noexcept {
    auto *m = (struct dap_step_out_request *)mem;
    dap_msg_hdr_init(&m->hdr, tag_id);
    return m;
}
static inline struct dap_step_out_response *dap_init_step_out_response(void *mem, AkU64 tag_id) noexcept {
    auto *m = (struct dap_step_out_response *)mem;
    dap_msg_hdr_init(&m->hdr, tag_id);
    return m;
}
static inline struct dap_pause_request *dap_init_pause_request(void *mem, AkU64 tag_id) noexcept {
    auto *m = (struct dap_pause_request *)mem;
    dap_msg_hdr_init(&m->hdr, tag_id);
    return m;
}
static inline struct dap_pause_response *dap_init_pause_response(void *mem, AkU64 tag_id) noexcept {
    auto *m = (struct dap_pause_response *)mem;
    dap_msg_hdr_init(&m->hdr, tag_id);
    return m;
}

static inline struct dap_step_back_request *dap_init_step_back_request(void *mem, AkU64 tag_id) noexcept {
    auto *m = (struct dap_step_back_request *)mem;
    dap_msg_hdr_init(&m->hdr, tag_id);
    return m;
}
static inline struct dap_step_back_response *dap_init_step_back_response(void *mem, AkU64 tag_id) noexcept {
    auto *m = (struct dap_step_back_response *)mem;
    dap_msg_hdr_init(&m->hdr, tag_id);
    return m;
}
static inline struct dap_reverse_continue_request *dap_init_reverse_continue_request(void *mem, AkU64 tag_id) noexcept {
    auto *m = (struct dap_reverse_continue_request *)mem;
    dap_msg_hdr_init(&m->hdr, tag_id);
    return m;
}
static inline struct dap_reverse_continue_response *dap_init_reverse_continue_response(void *mem, AkU64 tag_id) noexcept {
    auto *m = (struct dap_reverse_continue_response *)mem;
    dap_msg_hdr_init(&m->hdr, tag_id);
    return m;
}
static inline struct dap_restart_frame_request *dap_init_restart_frame_request(void *mem, AkU64 tag_id) noexcept {
    auto *m = (struct dap_restart_frame_request *)mem;
    dap_msg_hdr_init(&m->hdr, tag_id);
    return m;
}
static inline struct dap_restart_frame_response *dap_init_restart_frame_response(void *mem, AkU64 tag_id) noexcept {
    auto *m = (struct dap_restart_frame_response *)mem;
    dap_msg_hdr_init(&m->hdr, tag_id);
    return m;
}
static inline struct dap_goto_request *dap_init_goto_request(void *mem, AkU64 tag_id) noexcept {
    auto *m = (struct dap_goto_request *)mem;
    dap_msg_hdr_init(&m->hdr, tag_id);
    return m;
}
static inline struct dap_goto_response *dap_init_goto_response(void *mem, AkU64 tag_id) noexcept {
    auto *m = (struct dap_goto_response *)mem;
    dap_msg_hdr_init(&m->hdr, tag_id);
    return m;
}
