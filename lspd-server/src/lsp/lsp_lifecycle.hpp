#pragma once

#include "lsp_basic.hpp" // IWYU pragma: keep
#include "lsp_dyn.hpp"   // IWYU pragma: keep

/// \file lsp_lifecycle.hpp
/// \brief LSP lifecycle message declarations (initialize, shutdown, exit)

/// Initialize params/result and request/response/error
struct lsp_initialize_params {
    struct lsp_opt_uinteger process_id;
    struct lsp_client_info  client_info;
};
struct lsp_initialize_result {
};
struct lsp_initialize_request {
    struct lsp_msg_hdr hdr;
    struct lsp_initialize_params params;
};
struct lsp_initialize_partial_response {
    struct lsp_msg_hdr hdr;
};
struct lsp_initialize_response {
    struct lsp_msg_hdr hdr;
    struct lsp_initialize_result result;
};
struct lsp_initialize_error_result {
    struct lsp_msg_hdr hdr;
    struct lsp_error_result error;
};
static struct lsp_initialize_request*          lsp_init_initialize_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_initialize_partial_response* lsp_init_initialize_partial_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_initialize_response*         lsp_init_initialize_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_initialize_error_result*     lsp_init_initialize_error_result(void *mem, AkU64 tag_id) noexcept;

/// Shutdown request/response/error
struct lsp_shutdown_request { struct lsp_msg_hdr hdr; };
struct lsp_shutdown_partial_response { struct lsp_msg_hdr hdr; };
struct lsp_shutdown_response { struct lsp_msg_hdr hdr; };
struct lsp_shutdown_error_result { struct lsp_msg_hdr hdr; struct lsp_error_result error; };
static struct lsp_shutdown_request*          lsp_init_shutdown_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_shutdown_partial_response* lsp_init_shutdown_partial_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_shutdown_response*         lsp_init_shutdown_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_shutdown_error_result*     lsp_init_shutdown_error_result(void *mem, AkU64 tag_id) noexcept;

/// Initialized and exit notifications
struct lsp_initialized_notification { struct lsp_msg_hdr hdr; };
static struct lsp_initialized_notification* lsp_init_initialized_notification(void *mem, AkU64 tag_id) noexcept;
struct lsp_exit_notification { struct lsp_msg_hdr hdr; };
static struct lsp_exit_notification* lsp_init_exit_notification(void *mem, AkU64 tag_id) noexcept;
