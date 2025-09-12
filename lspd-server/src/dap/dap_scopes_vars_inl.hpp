#pragma once

#include "dap_scopes_vars.hpp"
#include "dap_basic_inl.hpp"

static inline struct dap_scopes_request *dap_init_scopes_request(void *mem, AkU64 tag_id) noexcept {
    auto *m = (struct dap_scopes_request *)mem;
    dap_msg_hdr_init(&m->hdr, tag_id);
    return m;
}
static inline struct dap_scopes_response *dap_init_scopes_response(void *mem, AkU64 tag_id) noexcept {
    auto *m = (struct dap_scopes_response *)mem;
    dap_msg_hdr_init(&m->hdr, tag_id);
    return m;
}

static inline struct dap_variables_request *dap_init_variables_request(void *mem, AkU64 tag_id) noexcept {
    auto *m = (struct dap_variables_request *)mem;
    dap_msg_hdr_init(&m->hdr, tag_id);
    return m;
}
static inline struct dap_variables_response *dap_init_variables_response(void *mem, AkU64 tag_id) noexcept {
    auto *m = (struct dap_variables_response *)mem;
    dap_msg_hdr_init(&m->hdr, tag_id);
    return m;
}

static inline struct dap_set_variable_request *dap_init_set_variable_request(void *mem, AkU64 tag_id) noexcept {
    auto *m = (struct dap_set_variable_request *)mem;
    dap_msg_hdr_init(&m->hdr, tag_id);
    return m;
}
static inline struct dap_set_variable_response *dap_init_set_variable_response(void *mem, AkU64 tag_id) noexcept {
    auto *m = (struct dap_set_variable_response *)mem;
    dap_msg_hdr_init(&m->hdr, tag_id);
    return m;
}

static inline struct dap_evaluate_request *dap_init_evaluate_request(void *mem, AkU64 tag_id) noexcept {
    auto *m = (struct dap_evaluate_request *)mem;
    dap_msg_hdr_init(&m->hdr, tag_id);
    return m;
}
static inline struct dap_evaluate_response *dap_init_evaluate_response(void *mem, AkU64 tag_id) noexcept {
    auto *m = (struct dap_evaluate_response *)mem;
    dap_msg_hdr_init(&m->hdr, tag_id);
    return m;
}

static inline struct dap_set_expression_request *dap_init_set_expression_request(void *mem, AkU64 tag_id) noexcept {
    auto *m = (struct dap_set_expression_request *)mem;
    dap_msg_hdr_init(&m->hdr, tag_id);
    return m;
}
static inline struct dap_set_expression_response *dap_init_set_expression_response(void *mem, AkU64 tag_id) noexcept {
    auto *m = (struct dap_set_expression_response *)mem;
    dap_msg_hdr_init(&m->hdr, tag_id);
    return m;
}

static inline struct dap_exception_info_request *dap_init_exception_info_request(void *mem, AkU64 tag_id) noexcept {
    auto *m = (struct dap_exception_info_request *)mem;
    dap_msg_hdr_init(&m->hdr, tag_id);
    return m;
}
static inline struct dap_exception_info_response *dap_init_exception_info_response(void *mem, AkU64 tag_id) noexcept {
    auto *m = (struct dap_exception_info_response *)mem;
    dap_msg_hdr_init(&m->hdr, tag_id);
    return m;
}

static inline struct dap_read_memory_request *dap_init_read_memory_request(void *mem, AkU64 tag_id) noexcept {
    auto *m = (struct dap_read_memory_request *)mem;
    dap_msg_hdr_init(&m->hdr, tag_id);
    return m;
}
static inline struct dap_read_memory_response *dap_init_read_memory_response(void *mem, AkU64 tag_id) noexcept {
    auto *m = (struct dap_read_memory_response *)mem;
    dap_msg_hdr_init(&m->hdr, tag_id);
    return m;
}

static inline struct dap_write_memory_request *dap_init_write_memory_request(void *mem, AkU64 tag_id) noexcept {
    auto *m = (struct dap_write_memory_request *)mem;
    dap_msg_hdr_init(&m->hdr, tag_id);
    return m;
}
static inline struct dap_write_memory_response *dap_init_write_memory_response(void *mem, AkU64 tag_id) noexcept {
    auto *m = (struct dap_write_memory_response *)mem;
    dap_msg_hdr_init(&m->hdr, tag_id);
    return m;
}

static inline struct dap_disassemble_request *dap_init_disassemble_request(void *mem, AkU64 tag_id) noexcept {
    auto *m = (struct dap_disassemble_request *)mem;
    dap_msg_hdr_init(&m->hdr, tag_id);
    return m;
}
static inline struct dap_disassemble_response *dap_init_disassemble_response(void *mem, AkU64 tag_id) noexcept {
    auto *m = (struct dap_disassemble_response *)mem;
    dap_msg_hdr_init(&m->hdr, tag_id);
    return m;
}
