#pragma once

#include "dap_basic.hpp"

/// \file dap_run.hpp
/// \brief Execution control messages (continue, next, stepIn, stepOut, pause)

// continue
struct dap_continue_arguments 
{
    int thread_id;
    bool single_thread;
};
struct dap_continue_request 
{
    struct dap_msg_hdr hdr;
    struct dap_continue_arguments arguments;
};
struct dap_continue_response 
{
    struct dap_msg_hdr hdr;
};
static struct dap_continue_request *dap_init_continue_request(void *mem, AkU64 tag_id) noexcept;
static struct dap_continue_response *dap_init_continue_response(void *mem, AkU64 tag_id) noexcept;

// next
struct dap_next_arguments 
{
    int thread_id;
    bool single_thread;
    int granularity;
};
struct dap_next_request 
{
    struct dap_msg_hdr hdr;
    struct dap_next_arguments arguments;
};
struct dap_next_response 
{
    struct dap_msg_hdr hdr;
};
static struct dap_next_request *dap_init_next_request(void *mem, AkU64 tag_id) noexcept;
static struct dap_next_response *dap_init_next_response(void *mem, AkU64 tag_id) noexcept;

// stepIn
struct dap_step_in_arguments 
{
    int thread_id;
    bool single_thread;
    int target_id;
    int granularity;
};
struct dap_step_in_request 
{
    struct dap_msg_hdr hdr;
    struct dap_step_in_arguments arguments;
};
struct dap_step_in_response 
{
    struct dap_msg_hdr hdr;
};
static struct dap_step_in_request *dap_init_step_in_request(void *mem, AkU64 tag_id) noexcept;
static struct dap_step_in_response *dap_init_step_in_response(void *mem, AkU64 tag_id) noexcept;

// stepOut
struct dap_step_out_arguments 
{
    int thread_id;
    bool single_thread;
    int granularity;
};
struct dap_step_out_request 
{
    struct dap_msg_hdr hdr;
    struct dap_step_out_arguments arguments;
};
struct dap_step_out_response 
{
    struct dap_msg_hdr hdr;
};
static struct dap_step_out_request *dap_init_step_out_request(void *mem, AkU64 tag_id) noexcept;
static struct dap_step_out_response *dap_init_step_out_response(void *mem, AkU64 tag_id) noexcept;

// pause
struct dap_pause_arguments 
{
    int thread_id;
};
struct dap_pause_request 
{
    struct dap_msg_hdr hdr;
    struct dap_pause_arguments arguments;
};
struct dap_pause_response 
{
    struct dap_msg_hdr hdr;
};
static struct dap_pause_request *dap_init_pause_request(void *mem, AkU64 tag_id) noexcept;
static struct dap_pause_response *dap_init_pause_response(void *mem, AkU64 tag_id) noexcept;

// stepBack
struct dap_step_back_arguments 
{
    int thread_id;
    bool single_thread;
    int granularity;
};
struct dap_step_back_request 
{
    struct dap_msg_hdr hdr;
    struct dap_step_back_arguments arguments;
};
struct dap_step_back_response 
{
    struct dap_msg_hdr hdr;
};
static struct dap_step_back_request *dap_init_step_back_request(void *mem, AkU64 tag_id) noexcept;
static struct dap_step_back_response *dap_init_step_back_response(void *mem, AkU64 tag_id) noexcept;

// reverseContinue
struct dap_reverse_continue_arguments 
{
    int thread_id;
    bool single_thread;
};
struct dap_reverse_continue_request 
{
    struct dap_msg_hdr hdr;
    struct dap_reverse_continue_arguments arguments;
};
struct dap_reverse_continue_response 
{
    struct dap_msg_hdr hdr;
};
static struct dap_reverse_continue_request *dap_init_reverse_continue_request(void *mem, AkU64 tag_id) noexcept;
static struct dap_reverse_continue_response *dap_init_reverse_continue_response(void *mem, AkU64 tag_id) noexcept;

// restartFrame
struct dap_restart_frame_arguments 
{
    int frame_id;
};
struct dap_restart_frame_request 
{
    struct dap_msg_hdr hdr;
    struct dap_restart_frame_arguments arguments;
};
struct dap_restart_frame_response 
{
    struct dap_msg_hdr hdr;
};
static struct dap_restart_frame_request *dap_init_restart_frame_request(void *mem, AkU64 tag_id) noexcept;
static struct dap_restart_frame_response *dap_init_restart_frame_response(void *mem, AkU64 tag_id) noexcept;

// goto
struct dap_goto_arguments 
{
    int thread_id;
    int target_id;
};
struct dap_goto_request 
{
    struct dap_msg_hdr hdr;
    struct dap_goto_arguments arguments;
};
struct dap_goto_response
{
    struct dap_msg_hdr hdr;
};
static struct dap_goto_request *dap_init_goto_request(void *mem, AkU64 tag_id) noexcept;
static struct dap_goto_response *dap_init_goto_response(void *mem, AkU64 tag_id) noexcept;
