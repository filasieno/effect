#pragma once

#include "dap_basic.hpp"

/// \file dap_breakpoints.hpp
/// \brief DAP breakpoint-related requests

// setBreakpoints
struct dap_set_breakpoints_arguments{
    int has_source;              // 0/1 if source provided
    const char* source_path;     // optional
    int source_reference;        // optional (>0 means use sourceReference)
    /* array<SourceBreakpoint> breakpoints; */
    int source_modified;         // bool
};
struct dap_set_breakpoints_response_body{ /* array<Breakpoint> breakpoints; */ };
struct dap_set_breakpoints_request  { struct dap_msg_hdr hdr; struct dap_set_breakpoints_arguments arguments; };
struct dap_set_breakpoints_response { struct dap_msg_hdr hdr; struct dap_set_breakpoints_response_body body; };
static struct dap_set_breakpoints_request*  dap_init_set_breakpoints_request(void* mem, AkU64 tag_id) noexcept;
static struct dap_set_breakpoints_response* dap_init_set_breakpoints_response(void* mem, AkU64 tag_id) noexcept;

// setFunctionBreakpoints
struct dap_set_function_breakpoints_arguments{ /* array<FunctionBreakpoint> breakpoints; */ };
struct dap_set_function_breakpoints_response_body{ /* array<Breakpoint> breakpoints; */ };
struct dap_set_function_breakpoints_request  { struct dap_msg_hdr hdr; struct dap_set_function_breakpoints_arguments arguments; };
struct dap_set_function_breakpoints_response { struct dap_msg_hdr hdr; struct dap_set_function_breakpoints_response_body body; };
static struct dap_set_function_breakpoints_request*  dap_init_set_function_breakpoints_request(void* mem, AkU64 tag_id) noexcept;
static struct dap_set_function_breakpoints_response* dap_init_set_function_breakpoints_response(void* mem, AkU64 tag_id) noexcept;

// setDataBreakpoints
struct dap_set_data_breakpoints_arguments{ /* array<DataBreakpoint> breakpoints; */ };
struct dap_set_data_breakpoints_response_body{ /* array<Breakpoint> breakpoints; */ };
struct dap_set_data_breakpoints_request  { struct dap_msg_hdr hdr; struct dap_set_data_breakpoints_arguments arguments; };
struct dap_set_data_breakpoints_response { struct dap_msg_hdr hdr; struct dap_set_data_breakpoints_response_body body; };
static struct dap_set_data_breakpoints_request*  dap_init_set_data_breakpoints_request(void* mem, AkU64 tag_id) noexcept;
static struct dap_set_data_breakpoints_response* dap_init_set_data_breakpoints_response(void* mem, AkU64 tag_id) noexcept;

// setExceptionBreakpoints
struct dap_set_exception_breakpoints_arguments{
    /* array<string> filters; */
    /* array<ExceptionFilterOptions> filterOptions; */
    /* array<ExceptionOptions> exceptionOptions; */
};
struct dap_set_exception_breakpoints_response_body{ /* array<Breakpoint> breakpoints; */ };
struct dap_set_exception_breakpoints_request  { struct dap_msg_hdr hdr; struct dap_set_exception_breakpoints_arguments arguments; };
struct dap_set_exception_breakpoints_response { struct dap_msg_hdr hdr; struct dap_set_exception_breakpoints_response_body body; };
static struct dap_set_exception_breakpoints_request*  dap_init_set_exception_breakpoints_request(void* mem, AkU64 tag_id) noexcept;
static struct dap_set_exception_breakpoints_response* dap_init_set_exception_breakpoints_response(void* mem, AkU64 tag_id) noexcept;

// breakpointLocations
struct dap_breakpoint_locations_arguments{
    int has_source;              // 0/1 if source provided
    const char* source_path;     // optional
    int source_reference;        // optional
    int line;
    int column;
    int end_line;
    int end_column;
};
struct dap_breakpoint_locations_response_body{ /* array<BreakpointLocation> breakpoints; */ };
struct dap_breakpoint_locations_request  { struct dap_msg_hdr hdr; struct dap_breakpoint_locations_arguments arguments; };
struct dap_breakpoint_locations_response { struct dap_msg_hdr hdr; struct dap_breakpoint_locations_response_body body; };
static struct dap_breakpoint_locations_request*  dap_init_breakpoint_locations_request(void* mem, AkU64 tag_id) noexcept;
static struct dap_breakpoint_locations_response* dap_init_breakpoint_locations_response(void* mem, AkU64 tag_id) noexcept;

// dataBreakpointInfo
struct dap_data_breakpoint_info_arguments{
    int variables_reference;     // optional
    const char* name;            // variable child or expression/address
    int frame_id;                // optional
    int bytes;                   // optional
    int as_address;              // bool optional
    const char* mode;            // optional
};
struct dap_data_breakpoint_info_response_body{
    const char* data_id;         // nullable
    const char* description;
    /* array<DataBreakpointAccessType> accessTypes; */
    int can_persist;             // bool
};
struct dap_data_breakpoint_info_request  { struct dap_msg_hdr hdr; struct dap_data_breakpoint_info_arguments arguments; };
struct dap_data_breakpoint_info_response { struct dap_msg_hdr hdr; struct dap_data_breakpoint_info_response_body body; };
static struct dap_data_breakpoint_info_request*  dap_init_data_breakpoint_info_request(void* mem, AkU64 tag_id) noexcept;
static struct dap_data_breakpoint_info_response* dap_init_data_breakpoint_info_response(void* mem, AkU64 tag_id) noexcept;

// setInstructionBreakpoints
struct dap_set_instruction_breakpoints_arguments{ /* array<InstructionBreakpoint> breakpoints; */ };
struct dap_set_instruction_breakpoints_response_body{ /* array<Breakpoint> breakpoints; */ };
struct dap_set_instruction_breakpoints_request  { struct dap_msg_hdr hdr; struct dap_set_instruction_breakpoints_arguments arguments; };
struct dap_set_instruction_breakpoints_response { struct dap_msg_hdr hdr; struct dap_set_instruction_breakpoints_response_body body; };
static struct dap_set_instruction_breakpoints_request*  dap_init_set_instruction_breakpoints_request(void* mem, AkU64 tag_id) noexcept;
static struct dap_set_instruction_breakpoints_response* dap_init_set_instruction_breakpoints_response(void* mem, AkU64 tag_id) noexcept;


