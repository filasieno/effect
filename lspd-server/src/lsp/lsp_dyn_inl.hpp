#pragma once

#include "lsp_dyn.hpp"

/// \brief Initialize an lsp_dyn as boolean
/// \param dyn Pointer to dynamic value to initialize
/// \param value Boolean value
static inline struct lsp_dyn_bool* lsp_dyn_init_bool(void* mem, bool value) noexcept {
    auto* dyn = (struct lsp_dyn_bool*)mem;
    dyn->kind = value ? LSP_DYN_BOOLEAN_TRUE : LSP_DYN_BOOLEAN_FALSE;
    return dyn;
}

/// \brief Initialize an lsp_dyn as integer
/// \param dyn Pointer to dynamic value to initialize
/// \param value Integer value
static inline struct lsp_dyn_int* lsp_dyn_init_int(void* mem, signed long long value) noexcept {
    auto* dyn = (struct lsp_dyn_int*)mem;
    dyn->kind = LSP_DYN_INTEGER;
    dyn->value = value;
    return dyn;
}

/// \brief Initialize an lsp_dyn as unsigned integer
/// \param dyn Pointer to dynamic value to initialize
/// \param value Unsigned integer value
static inline struct lsp_dyn_uint* lsp_dyn_init_uint(void* mem, unsigned long long value) noexcept {
    auto* dyn = (struct lsp_dyn_uint*)mem;
    dyn->kind = LSP_DYN_UINTEGER;
    dyn->value = value;
    return dyn;
}

/// \brief Initialize an lsp_dyn as string
/// \param dyn Pointer to dynamic value to initialize
/// \param value String value
static inline struct lsp_dyn_string* lsp_dyn_init_string(void* mem, const lsp_string* value) noexcept {
    auto* dyn = (struct lsp_dyn_string*)mem;
    dyn->kind = LSP_DYN_STRING;
    dyn->value = *value;
    return dyn;
}

/// \brief Get the type of an lsp_dyn value
/// \param dyn Pointer to dynamic value
/// \return Dynamic value kind
static inline enum lsp_dyn_kind lsp_dyn_get_kind(const struct lsp_dyn* dyn) noexcept {
    return (enum lsp_dyn_kind)dyn->kind;
}