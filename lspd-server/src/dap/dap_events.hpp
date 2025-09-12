#pragma once

#include "dap_basic.hpp"

/// \file dap_events.hpp
/// \brief DAP events sent from debug adapter to client (placeholders)

// Local typed placeholders for events
struct dap_stopped_event_body{};
struct dap_continued_event_body{};
struct dap_output_event_body{};
struct dap_terminated_event_body{};
struct dap_exited_event_body{};
struct dap_thread_event_body{};
struct dap_breakpoint_event_body{};
struct dap_process_event_body{};
struct dap_module_event_body{};
struct dap_loaded_source_event_body{};
struct dap_capabilities_event_body{};
struct dap_progress_start_event_body{};
struct dap_progress_update_event_body{};
struct dap_progress_end_event_body{};
struct dap_invalidated_event_body{};
struct dap_memory_event_body{};

struct dap_stopped_event      { struct dap_msg_hdr hdr; struct dap_stopped_event_body body; };
struct dap_continued_event    { struct dap_msg_hdr hdr; struct dap_continued_event_body body; };
struct dap_output_event       { struct dap_msg_hdr hdr; struct dap_output_event_body body; };
struct dap_terminated_event   { struct dap_msg_hdr hdr; struct dap_terminated_event_body body; };
struct dap_exited_event       { struct dap_msg_hdr hdr; struct dap_exited_event_body body; };
struct dap_thread_event       { struct dap_msg_hdr hdr; struct dap_thread_event_body body; };
struct dap_breakpoint_event   { struct dap_msg_hdr hdr; struct dap_breakpoint_event_body body; };
struct dap_process_event      { struct dap_msg_hdr hdr; struct dap_process_event_body body; };
struct dap_module_event       { struct dap_msg_hdr hdr; struct dap_module_event_body body; };
struct dap_loaded_source_event{ struct dap_msg_hdr hdr; struct dap_loaded_source_event_body body; };

static struct dap_stopped_event*        dap_init_stopped_event(void* mem, AkU64 tag_id) noexcept;
static struct dap_continued_event*      dap_init_continued_event(void* mem, AkU64 tag_id) noexcept;
static struct dap_output_event*         dap_init_output_event(void* mem, AkU64 tag_id) noexcept;
static struct dap_terminated_event*     dap_init_terminated_event(void* mem, AkU64 tag_id) noexcept;
static struct dap_exited_event*         dap_init_exited_event(void* mem, AkU64 tag_id) noexcept;
static struct dap_thread_event*         dap_init_thread_event(void* mem, AkU64 tag_id) noexcept;
static struct dap_breakpoint_event*     dap_init_breakpoint_event(void* mem, AkU64 tag_id) noexcept;
static struct dap_process_event*        dap_init_process_event(void* mem, AkU64 tag_id) noexcept;
static struct dap_module_event*         dap_init_module_event(void* mem, AkU64 tag_id) noexcept;
static struct dap_loaded_source_event*  dap_init_loaded_source_event(void* mem, AkU64 tag_id) noexcept;

// additional events
struct dap_capabilities_event { struct dap_msg_hdr hdr; struct dap_capabilities_event_body body; };
struct dap_progress_start_event { struct dap_msg_hdr hdr; struct dap_progress_start_event_body body; };
struct dap_progress_update_event{ struct dap_msg_hdr hdr; struct dap_progress_update_event_body body; };
struct dap_progress_end_event   { struct dap_msg_hdr hdr; struct dap_progress_end_event_body body; };
struct dap_invalidated_event    { struct dap_msg_hdr hdr; struct dap_invalidated_event_body body; };
struct dap_memory_event         { struct dap_msg_hdr hdr; struct dap_memory_event_body body; };

static struct dap_capabilities_event*   dap_init_capabilities_event(void* mem, AkU64 tag_id) noexcept;
static struct dap_progress_start_event* dap_init_progress_start_event(void* mem, AkU64 tag_id) noexcept;
static struct dap_progress_update_event* dap_init_progress_update_event(void* mem, AkU64 tag_id) noexcept;
static struct dap_progress_end_event*   dap_init_progress_end_event(void* mem, AkU64 tag_id) noexcept;
static struct dap_invalidated_event*    dap_init_invalidated_event(void* mem, AkU64 tag_id) noexcept;
static struct dap_memory_event*         dap_init_memory_event(void* mem, AkU64 tag_id) noexcept;


