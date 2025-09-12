#pragma once

#include "dap_stack.hpp"
#include "dap_basic_inl.hpp"

static inline struct dap_stack_trace_request* dap_init_stack_trace_request(void* mem, AkU64 tag_id) noexcept {
    auto* m = (struct dap_stack_trace_request*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m;
}
static inline struct dap_stack_trace_response* dap_init_stack_trace_response(void* mem, AkU64 tag_id) noexcept {
    auto* m = (struct dap_stack_trace_response*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m;
}

static inline struct dap_source_request* dap_init_source_request(void* mem, AkU64 tag_id) noexcept { auto* m = (struct dap_source_request*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m; }
static inline struct dap_source_response* dap_init_source_response(void* mem, AkU64 tag_id) noexcept { auto* m = (struct dap_source_response*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m; }

static inline struct dap_modules_request* dap_init_modules_request(void* mem, AkU64 tag_id) noexcept { auto* m = (struct dap_modules_request*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m; }
static inline struct dap_modules_response* dap_init_modules_response(void* mem, AkU64 tag_id) noexcept { auto* m = (struct dap_modules_response*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m; }

static inline struct dap_loaded_sources_request* dap_init_loaded_sources_request(void* mem, AkU64 tag_id) noexcept { auto* m = (struct dap_loaded_sources_request*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m; }
static inline struct dap_loaded_sources_response* dap_init_loaded_sources_response(void* mem, AkU64 tag_id) noexcept { auto* m = (struct dap_loaded_sources_response*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m; }

static inline struct dap_step_in_targets_request* dap_init_step_in_targets_request(void* mem, AkU64 tag_id) noexcept { auto* m = (struct dap_step_in_targets_request*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m; }
static inline struct dap_step_in_targets_response* dap_init_step_in_targets_response(void* mem, AkU64 tag_id) noexcept { auto* m = (struct dap_step_in_targets_response*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m; }

static inline struct dap_goto_targets_request* dap_init_goto_targets_request(void* mem, AkU64 tag_id) noexcept { auto* m = (struct dap_goto_targets_request*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m; }
static inline struct dap_goto_targets_response* dap_init_goto_targets_response(void* mem, AkU64 tag_id) noexcept { auto* m = (struct dap_goto_targets_response*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m; }

static inline struct dap_completions_request* dap_init_completions_request(void* mem, AkU64 tag_id) noexcept { auto* m = (struct dap_completions_request*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m; }
static inline struct dap_completions_response* dap_init_completions_response(void* mem, AkU64 tag_id) noexcept { auto* m = (struct dap_completions_response*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m; }


