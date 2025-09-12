#pragma once

#include "dap_basic.hpp"


// stackTrace
struct dap_stack_trace_arguments 
{
    int thread_id;
    int start_frame;
    int levels;
    int format_hex;
};

struct dap_stack_trace_response_body { 
    /* array<StackFrame> stackFrames; */
    int total_frames;
};

struct dap_stack_trace_request
{
    struct dap_msg_hdr hdr;
    struct dap_stack_trace_arguments arguments;
};

struct dap_stack_trace_response
{
    struct dap_msg_hdr hdr;
    struct dap_stack_trace_response_body body;
};

static struct dap_stack_trace_request *dap_init_stack_trace_request(void *mem, AkU64 tag_id) noexcept;
static struct dap_stack_trace_response *dap_init_stack_trace_response(void *mem, AkU64 tag_id) noexcept;

// source
struct dap_source_arguments
{
    int source_reference; /* Source source; */
};

struct dap_source_response_body
{
    const char *content;
    const char *mime_type;
};

struct dap_source_request
{
    struct dap_msg_hdr hdr;
    struct dap_source_arguments arguments;
};

struct dap_source_response
{
    struct dap_msg_hdr hdr;
    struct dap_source_response_body body;
};

static struct dap_source_request *dap_init_source_request(void *mem, AkU64 tag_id) noexcept;
static struct dap_source_response *dap_init_source_response(void *mem, AkU64 tag_id) noexcept;

// modules
struct dap_modules_arguments
{
    int start_module;
    int module_count;
};

struct dap_modules_response_body
{ /* array<Module> modules; */
    int total_modules;
};

struct dap_modules_request
{
    struct dap_msg_hdr hdr;
    struct dap_modules_arguments arguments;
};

struct dap_modules_response
{
    struct dap_msg_hdr hdr;
    struct dap_modules_response_body body;
};

static struct dap_modules_request *dap_init_modules_request(void *mem, AkU64 tag_id) noexcept;
static struct dap_modules_response *dap_init_modules_response(void *mem, AkU64 tag_id) noexcept;

// loadedSources
struct dap_loaded_sources_arguments
{
    int unused;
};

struct dap_loaded_sources_response_body
{ /* array<Source> sources; */
};

struct dap_loaded_sources_request
{
    struct dap_msg_hdr hdr;
    struct dap_loaded_sources_arguments arguments;
};

struct dap_loaded_sources_response
{
    struct dap_msg_hdr hdr;
    struct dap_loaded_sources_response_body body;
};

static struct dap_loaded_sources_request *dap_init_loaded_sources_request(void *mem, AkU64 tag_id) noexcept;
static struct dap_loaded_sources_response *dap_init_loaded_sources_response(void *mem, AkU64 tag_id) noexcept;

// stepInTargets
struct dap_step_in_targets_arguments
{
    int frame_id;
};

struct dap_step_in_targets_response_body
{ /* array<StepInTarget> targets; */
};

struct dap_step_in_targets_request
{
    struct dap_msg_hdr hdr;
    struct dap_step_in_targets_arguments arguments;
};

struct dap_step_in_targets_response
{
    struct dap_msg_hdr hdr;
    struct dap_step_in_targets_response_body body;
};

static struct dap_step_in_targets_request *dap_init_step_in_targets_request(void *mem, AkU64 tag_id) noexcept;
static struct dap_step_in_targets_response *dap_init_step_in_targets_response(void *mem, AkU64 tag_id) noexcept;

// gotoTargets
struct dap_goto_targets_arguments
{ /* Source source; */
    int line;
    int column;
};

struct dap_goto_targets_response_body
{ /* array<GotoTarget> targets; */
};

struct dap_goto_targets_request
{
    struct dap_msg_hdr hdr;
    struct dap_goto_targets_arguments arguments;
};

struct dap_goto_targets_response
{
    struct dap_msg_hdr hdr;
    struct dap_goto_targets_response_body body;
};

static struct dap_goto_targets_request *dap_init_goto_targets_request(void *mem, AkU64 tag_id) noexcept;
static struct dap_goto_targets_response *dap_init_goto_targets_response(void *mem, AkU64 tag_id) noexcept;

// completions
struct dap_completions_arguments
{
    int frame_id;
    const char *text;
    int column;
    int line;
};

struct dap_completions_response_body
{ /* array<CompletionItem> targets; */
};

struct dap_completions_request
{
    struct dap_msg_hdr hdr;
    struct dap_completions_arguments arguments;
};

struct dap_completions_response
{
    struct dap_msg_hdr hdr;
    struct dap_completions_response_body body;
};

static struct dap_completions_request *dap_init_completions_request(void *mem, AkU64 tag_id) noexcept;
static struct dap_completions_response *dap_init_completions_response(void *mem, AkU64 tag_id) noexcept;
