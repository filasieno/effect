#pragma once

#include "lsp_lifecycle.hpp"

/// \file lsp_lifecycle_inl.hpp
/// \brief Inline implementations for lifecycle LSP types
///
/// This file contains inline function implementations for the LSP lifecycle types
/// defined in lsp_lifecycle.hpp.

// ======================================================================================================================
// INITIALIZE REQUEST/RESPONSE UTILITIES
// ======================================================================================================================

/// \brief Initialize an lsp_initialize_params structure
/// \param params Pointer to params to initialize
/// \param process_id Optional process ID
/// \param client_info Client information
static inline void lsp_initialize_params_init(lsp_initialize_params* params, lsp_opt_uinteger process_id, const lsp_client_info* client_info) noexcept {
    params->process_id = process_id;
    params->client_info = *client_info;
}

/// \brief Initialize an lsp_initialize_result structure (empty)
/// \param result Pointer to result (no initialization needed)
static inline void lsp_initialize_result_init(lsp_initialize_result* result) noexcept {
    // Empty - no fields to initialize
    (void)result; // Suppress unused parameter warning
}

// ======================================================================================================================
// CLIENT INFO UTILITIES
// ======================================================================================================================

/// \brief Initialize an lsp_client_info structure
/// \param info Pointer to client info to initialize
/// \param name Client name
/// \param version Optional client version
static inline void lsp_client_info_init(lsp_client_info* info, const lsp_string* name, const lsp_opt_string* version) noexcept {
    info->name = *name;
    info->version = *version;
}
