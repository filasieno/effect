#pragma once

#include "dap_basic.hpp"
/// Also see inline initializers in `dap_threads_inl.hpp`

/// \file dap_threads.hpp
/// \brief DAP thread enumeration and thread-related requests

// threads
struct dap_locations_arguments{}; // empty placeholder for 'no args'
struct dap_threads_response_body{};
struct dap_threads_request { struct dap_msg_hdr hdr; struct dap_locations_arguments arguments; };
struct dap_threads_response { struct dap_msg_hdr hdr; struct dap_threads_response_body body; };
static struct dap_threads_request*  dap_init_threads_request(void* mem, AkU64 tag_id) noexcept;
static struct dap_threads_response* dap_init_threads_response(void* mem, AkU64 tag_id) noexcept;

// terminateThreads
struct dap_terminate_threads_arguments{};
struct dap_terminate_threads_request { struct dap_msg_hdr hdr; struct dap_terminate_threads_arguments arguments; };
struct dap_terminate_threads_response{ struct dap_msg_hdr hdr; };
static struct dap_terminate_threads_request*  dap_init_terminate_threads_request(void* mem, AkU64 tag_id) noexcept;
static struct dap_terminate_threads_response* dap_init_terminate_threads_response(void* mem, AkU64 tag_id) noexcept;


