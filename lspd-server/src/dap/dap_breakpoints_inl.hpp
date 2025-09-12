#pragma once

#include "dap_breakpoints.hpp"
#include "dap_basic_inl.hpp"

static inline struct dap_set_breakpoints_request* dap_init_set_breakpoints_request(void* mem, AkU64 tag_id) noexcept { auto* m = (struct dap_set_breakpoints_request*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m; }
static inline struct dap_set_breakpoints_response* dap_init_set_breakpoints_response(void* mem, AkU64 tag_id) noexcept { auto* m = (struct dap_set_breakpoints_response*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m; }

static inline struct dap_set_function_breakpoints_request* dap_init_set_function_breakpoints_request(void* mem, AkU64 tag_id) noexcept { auto* m = (struct dap_set_function_breakpoints_request*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m; }
static inline struct dap_set_function_breakpoints_response* dap_init_set_function_breakpoints_response(void* mem, AkU64 tag_id) noexcept { auto* m = (struct dap_set_function_breakpoints_response*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m; }

static inline struct dap_set_data_breakpoints_request* dap_init_set_data_breakpoints_request(void* mem, AkU64 tag_id) noexcept { auto* m = (struct dap_set_data_breakpoints_request*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m; }
static inline struct dap_set_data_breakpoints_response* dap_init_set_data_breakpoints_response(void* mem, AkU64 tag_id) noexcept { auto* m = (struct dap_set_data_breakpoints_response*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m; }

static inline struct dap_set_exception_breakpoints_request* dap_init_set_exception_breakpoints_request(void* mem, AkU64 tag_id) noexcept { auto* m = (struct dap_set_exception_breakpoints_request*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m; }
static inline struct dap_set_exception_breakpoints_response* dap_init_set_exception_breakpoints_response(void* mem, AkU64 tag_id) noexcept { auto* m = (struct dap_set_exception_breakpoints_response*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m; }

static inline struct dap_breakpoint_locations_request* dap_init_breakpoint_locations_request(void* mem, AkU64 tag_id) noexcept { auto* m = (struct dap_breakpoint_locations_request*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m; }
static inline struct dap_breakpoint_locations_response* dap_init_breakpoint_locations_response(void* mem, AkU64 tag_id) noexcept { auto* m = (struct dap_breakpoint_locations_response*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m; }

static inline struct dap_data_breakpoint_info_request* dap_init_data_breakpoint_info_request(void* mem, AkU64 tag_id) noexcept { auto* m = (struct dap_data_breakpoint_info_request*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m; }
static inline struct dap_data_breakpoint_info_response* dap_init_data_breakpoint_info_response(void* mem, AkU64 tag_id) noexcept { auto* m = (struct dap_data_breakpoint_info_response*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m; }

static inline struct dap_set_instruction_breakpoints_request* dap_init_set_instruction_breakpoints_request(void* mem, AkU64 tag_id) noexcept { auto* m = (struct dap_set_instruction_breakpoints_request*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m; }
static inline struct dap_set_instruction_breakpoints_response* dap_init_set_instruction_breakpoints_response(void* mem, AkU64 tag_id) noexcept { auto* m = (struct dap_set_instruction_breakpoints_response*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m; }


