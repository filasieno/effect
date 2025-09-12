#pragma once

#include "lsp_basic.hpp" // IWYU pragma: keep
#include "lsp_dyn.hpp"   // IWYU pragma: keep

/// \file lsp_lifecycle.hpp
/// \brief LSP lifecycle message declarations (initialize, shutdown, exit)

/// Client information supplied during initialization (moved from lsp_basic.hpp)
struct lsp_client_info {
    struct lsp_string     name;    ///< Human-readable client name
    struct lsp_opt_string version; ///< Optional client version
};

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

/// Capability registration params (moved from lsp_basic.hpp)
struct lsp_registration {
    struct lsp_string id;
    struct lsp_string method;
    // struct lsp_dyn register_options;
};

struct lsp_unregisteration {
    struct lsp_string id;
    struct lsp_string method;
};

/// The `client/registerCapability` request is sent from the server to the client to register for capabilities dynamically.
/// Params: RegistrationParams. Result: null.
struct lsp_client_register_capability_request {
    struct lsp_msg_hdr hdr;
    struct lsp_list<struct lsp_registration>* registrations;
};
struct lsp_client_register_capability_response {
    struct lsp_msg_hdr hdr;
};
struct lsp_client_register_capability_error_result {
    struct lsp_msg_hdr hdr;
    struct lsp_error_result error;
};
static struct lsp_client_register_capability_request*      lsp_init_client_register_capability_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_client_register_capability_response*     lsp_init_client_register_capability_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_client_register_capability_error_result* lsp_init_client_register_capability_error_result(void *mem, AkU64 tag_id) noexcept;

/// The `client/unregisterCapability` request is sent from the server to the client to unregister capabilities.
/// Params: UnregistrationParams. Result: null.
struct lsp_client_unregister_capability_request {
    struct lsp_msg_hdr hdr;
    struct lsp_list<struct lsp_unregisteration>* unregisterations;
};
struct lsp_client_unregister_capability_response {
    struct lsp_msg_hdr hdr;
};
struct lsp_client_unregister_capability_error_result {
    struct lsp_msg_hdr hdr;
    struct lsp_error_result error;
};
static struct lsp_client_unregister_capability_request*      lsp_init_client_unregister_capability_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_client_unregister_capability_response*     lsp_init_client_unregister_capability_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_client_unregister_capability_error_result* lsp_init_client_unregister_capability_error_result(void *mem, AkU64 tag_id) noexcept;

/// Registration params containers (moved from lsp_basic.hpp)
struct lsp_registration_params {
    struct lsp_list<struct lsp_registration>* registrations;
};

struct lsp_unregisteration_params {
    struct lsp_list<struct lsp_unregisteration>* unregisterations;
};
