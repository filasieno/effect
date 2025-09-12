#pragma once

#include "dap_basic.hpp"

/// \file dap_lifecycle.hpp
/// \brief DAP lifecycle messages (initialize, initialized, cancel, disconnect)

// Local typed argument/body placeholders for lifecycle











// initialize request/response
struct dap_initialize_request_arguments {
    const char* client_id;                // optional
    const char* client_name;              // optional
    const char* adapter_id;               // required
    const char* locale;                   // optional
    int lines_start_at_1;                 // bool
    int columns_start_at_1;               // bool
    const char* path_format;              // "path" | "uri"
    int supports_variable_type;           // bool
    int supports_variable_paging;         // bool
    int supports_run_in_terminal_request; // bool
    int supports_memory_references;       // bool
    int supports_progress_reporting;      // bool
    int supports_invalidated_event;       // bool
    int supports_memory_event;            // bool
    int supports_args_shell;              // bool (argsCanBeInterpretedByShell)
    int supports_start_debugging_request; // bool
    int supports_ansi_styling;            // bool
};

struct dap_initialize_request {
    struct dap_msg_hdr hdr;
    struct dap_initialize_request_arguments arguments;
};
struct dap_capabilities {};
struct dap_initialize_response {
    struct dap_msg_hdr hdr;
    struct dap_capabilities body;
};
struct dap_initialize_error_result {
    struct dap_msg_hdr hdr;
    struct dap_string message;
};
static struct dap_initialize_request *dap_init_initialize_request(void *mem, AkU64 tag_id) noexcept;
static struct dap_initialize_response *dap_init_initialize_response(void *mem, AkU64 tag_id) noexcept;
static struct dap_initialize_error_result *dap_init_initialize_error_result(void *mem, AkU64 tag_id) noexcept;

// initialized (event)
struct dap_initialized_event {
    struct dap_msg_hdr hdr;
};
static struct dap_initialized_event *dap_init_initialized_event(void *mem, AkU64 tag_id) noexcept;

// cancel request
struct dap_cancel_arguments {
    int request_id;           // optional
    const char* progress_id;  // optional
};
struct dap_cancel_request {
    struct dap_msg_hdr hdr;
    struct dap_cancel_arguments arguments;
};
static struct dap_cancel_request *dap_init_cancel_request(void *mem, AkU64 tag_id) noexcept;

// disconnect request/response
struct dap_disconnect_arguments {
    int restart;                // bool optional
    int terminate_debuggee;     // bool optional
    int suspend_debuggee;       // bool optional
};
struct dap_disconnect_request {
    struct dap_msg_hdr hdr;
    struct dap_disconnect_arguments arguments;
};
struct dap_disconnect_response {
    struct dap_msg_hdr hdr;
};
static struct dap_disconnect_request *dap_init_disconnect_request(void *mem, AkU64 tag_id) noexcept;
static struct dap_disconnect_response *dap_init_disconnect_response(void *mem, AkU64 tag_id) noexcept;

// configurationDone request/response
struct dap_configuration_done_arguments {};
struct dap_configuration_done_request {
    struct dap_msg_hdr hdr;
    struct dap_configuration_done_arguments arguments;
};
struct dap_configuration_done_response {
    struct dap_msg_hdr hdr;
};
static struct dap_configuration_done_request *dap_init_configuration_done_request(void *mem, AkU64 tag_id) noexcept;
static struct dap_configuration_done_response *dap_init_configuration_done_response(void *mem, AkU64 tag_id) noexcept;

// launch request/response
struct dap_launch_request_arguments {
    int no_debug;   // bool optional
    int has_restart;// bool optional (presence of __restart)
};
struct dap_launch_request {
    struct dap_msg_hdr hdr;
    struct dap_launch_request_arguments arguments;
};
struct dap_launch_response {
    struct dap_msg_hdr hdr;
};
static struct dap_launch_request *dap_init_launch_request(void *mem, AkU64 tag_id) noexcept;
static struct dap_launch_response *dap_init_launch_response(void *mem, AkU64 tag_id) noexcept;

// attach request/response
struct dap_attach_request_arguments {
    int has_restart;// bool optional (presence of __restart)
};
struct dap_attach_request {
    struct dap_msg_hdr hdr;
    struct dap_attach_request_arguments arguments;
};
struct dap_attach_response {
    struct dap_msg_hdr hdr;
};
static struct dap_attach_request *dap_init_attach_request(void *mem, AkU64 tag_id) noexcept;
static struct dap_attach_response *dap_init_attach_response(void *mem, AkU64 tag_id) noexcept;

// restart request/response
struct dap_restart_arguments {
    int is_launch; // 1 for launch args, 0 for attach args (hint)
};
struct dap_restart_request {
    struct dap_msg_hdr hdr;
    struct dap_restart_arguments arguments;
};
struct dap_restart_response {
    struct dap_msg_hdr hdr;
};
static struct dap_restart_request *dap_init_restart_request(void *mem, AkU64 tag_id) noexcept;
static struct dap_restart_response *dap_init_restart_response(void *mem, AkU64 tag_id) noexcept;

// terminate request/response
struct dap_terminate_arguments {
    int restart; // bool optional
};
struct dap_terminate_request {
    struct dap_msg_hdr hdr;
    struct dap_terminate_arguments arguments;
};
struct dap_terminate_response {
    struct dap_msg_hdr hdr;
};
static struct dap_terminate_request *dap_init_terminate_request(void *mem, AkU64 tag_id) noexcept;
static struct dap_terminate_response *dap_init_terminate_response(void *mem, AkU64 tag_id) noexcept;

// runInTerminal request/response (adapter -> client)
struct dap_run_in_terminal_request_arguments {
    const char* kind;   // "integrated" | "external"
    const char* title;  // optional
    const char* cwd;    // required by spec
    int args_count;     // placeholder for args length
    int args_can_be_interpreted_by_shell; // bool optional
};
struct dap_run_in_terminal_request {
    struct dap_msg_hdr hdr;
    struct dap_run_in_terminal_request_arguments arguments;
};
struct dap_run_in_terminal_response_body {
    int process_id;       // optional
    int shell_process_id; // optional
};
struct dap_run_in_terminal_response {
    struct dap_msg_hdr hdr;
    struct dap_run_in_terminal_response_body body;
};
static struct dap_run_in_terminal_request *dap_init_run_in_terminal_request(void *mem, AkU64 tag_id) noexcept;
static struct dap_run_in_terminal_response *dap_init_run_in_terminal_response(void *mem, AkU64 tag_id) noexcept;
