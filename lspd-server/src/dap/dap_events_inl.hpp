#pragma once

#include "dap_events.hpp"
#include "dap_basic_inl.hpp"

static inline struct dap_stopped_event* dap_init_stopped_event(void* mem, AkU64 tag_id) noexcept { auto* m = (struct dap_stopped_event*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m; }
static inline struct dap_continued_event* dap_init_continued_event(void* mem, AkU64 tag_id) noexcept { auto* m = (struct dap_continued_event*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m; }
static inline struct dap_output_event* dap_init_output_event(void* mem, AkU64 tag_id) noexcept { auto* m = (struct dap_output_event*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m; }
static inline struct dap_terminated_event* dap_init_terminated_event(void* mem, AkU64 tag_id) noexcept { auto* m = (struct dap_terminated_event*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m; }
static inline struct dap_exited_event* dap_init_exited_event(void* mem, AkU64 tag_id) noexcept { auto* m = (struct dap_exited_event*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m; }
static inline struct dap_thread_event* dap_init_thread_event(void* mem, AkU64 tag_id) noexcept { auto* m = (struct dap_thread_event*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m; }
static inline struct dap_breakpoint_event* dap_init_breakpoint_event(void* mem, AkU64 tag_id) noexcept { auto* m = (struct dap_breakpoint_event*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m; }
static inline struct dap_process_event* dap_init_process_event(void* mem, AkU64 tag_id) noexcept { auto* m = (struct dap_process_event*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m; }
static inline struct dap_module_event* dap_init_module_event(void* mem, AkU64 tag_id) noexcept { auto* m = (struct dap_module_event*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m; }
static inline struct dap_loaded_source_event* dap_init_loaded_source_event(void* mem, AkU64 tag_id) noexcept { auto* m = (struct dap_loaded_source_event*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m; }

static inline struct dap_capabilities_event* dap_init_capabilities_event(void* mem, AkU64 tag_id) noexcept { auto* m = (struct dap_capabilities_event*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m; }
static inline struct dap_progress_start_event* dap_init_progress_start_event(void* mem, AkU64 tag_id) noexcept { auto* m = (struct dap_progress_start_event*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m; }
static inline struct dap_progress_update_event* dap_init_progress_update_event(void* mem, AkU64 tag_id) noexcept { auto* m = (struct dap_progress_update_event*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m; }
static inline struct dap_progress_end_event* dap_init_progress_end_event(void* mem, AkU64 tag_id) noexcept { auto* m = (struct dap_progress_end_event*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m; }
static inline struct dap_invalidated_event* dap_init_invalidated_event(void* mem, AkU64 tag_id) noexcept { auto* m = (struct dap_invalidated_event*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m; }
static inline struct dap_memory_event* dap_init_memory_event(void* mem, AkU64 tag_id) noexcept { auto* m = (struct dap_memory_event*)mem; dap_msg_hdr_init(&m->hdr, tag_id); return m; }


