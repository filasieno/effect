#pragma once

#include "lsp_basic.hpp"

/// \file lsp_basic_inl.hpp
/// \brief Inline implementations for basic LSP types
///
/// This file contains inline function implementations for the LSP protocol types
/// defined in lsp_basic.hpp.

// ======================================================================================================================
// STRING UTILITIES
// ======================================================================================================================

/// \brief Initialize an lsp_string from a C string
/// \param str Pointer to lsp_string to initialize
/// \param c_str C string to copy from (must be null-terminated)
static inline void lsp_string_init(lsp_string* str, const char* c_str) noexcept {
    str->chars = c_str;
    str->length = (int)strlen(c_str);
}

/// \brief Check if two lsp_strings are equal
/// \param a First string to compare
/// \param b Second string to compare
/// \return true if strings are equal
static inline bool lsp_string_equal(const lsp_string* a, const lsp_string* b) noexcept {
    if (a->length != b->length) return false;
    return memcmp(a->chars, b->chars, (size_t)a->length) == 0;
}

// ======================================================================================================================
// POSITION UTILITIES
// ======================================================================================================================

/// \brief Initialize an lsp_position
/// \param pos Pointer to lsp_position to initialize
/// \param line Line number (0-based)
/// \param character Character position within the line (0-based)
static inline void lsp_position_init(lsp_position* pos, int line, int character) noexcept {
    pos->line = line;
    pos->character = character;
}

/// \brief Check if two positions are equal
/// \param a First position to compare
/// \param b Second position to compare
/// \return true if positions are equal
static inline bool lsp_position_equal(const lsp_position* a, const lsp_position* b) noexcept {
    return a->line == b->line && a->character == b->character;
}

// ======================================================================================================================
// RANGE UTILITIES
// ======================================================================================================================

/// \brief Initialize an lsp_range
/// \param range Pointer to lsp_range to initialize
/// \param start Start position
/// \param end End position (exclusive)
static inline void lsp_range_init(lsp_range* range, const lsp_position* start, const lsp_position* end) noexcept {
    range->start = *start;
    range->end = *end;
}

/// \brief Check if two ranges are equal
/// \param a First range to compare
/// \param b Second range to compare
/// \return true if ranges are equal
static inline bool lsp_range_equal(const lsp_range* a, const lsp_range* b) noexcept {
    return lsp_position_equal(&a->start, &b->start) && lsp_position_equal(&a->end, &b->end);
}

// ======================================================================================================================
// URI UTILITIES
// ======================================================================================================================

/// \brief Initialize an lsp_uri from a C string
/// \param uri Pointer to lsp_uri to initialize
/// \param c_str C string containing the URI (must be null-terminated)
static inline void lsp_uri_init(lsp_uri* uri, const char* c_str) noexcept {
    uri->chars = c_str;
}

/// \brief Check if two URIs are equal
/// \param a First URI to compare
/// \param b Second URI to compare
/// \return true if URIs are equal
static inline bool lsp_uri_equal(const lsp_uri* a, const lsp_uri* b) noexcept {
    return strcmp(a->chars, b->chars) == 0;
}

// ======================================================================================================================
// LOCATION UTILITIES
// ======================================================================================================================

/// \brief Initialize an lsp_location
/// \param loc Pointer to lsp_location to initialize
/// \param uri Document URI
/// \param range Text range within the document
static inline void lsp_location_init(lsp_location* loc, const lsp_uri* uri, const lsp_range* range) noexcept {
    loc->document_uri.chars = uri->chars;
    loc->range = *range;
}

/// \brief Check if two locations are equal
/// \param a First location to compare
/// \param b Second location to compare
/// \return true if locations are equal
static inline bool lsp_location_equal(const lsp_location* a, const lsp_location* b) noexcept {
    return lsp_uri_equal(&a->document_uri, &b->document_uri) && lsp_range_equal(&a->range, &b->range);
}

// ======================================================================================================================
// MESSAGE HEADER UTILITIES
// ======================================================================================================================

/// \brief Create an operation tag from method and interaction
/// \param method Message method type
/// \param interaction Message interaction kind
/// \return Combined operation tag
static inline AkU64 lsp_make_op_tag(lsp_method_type method, lsp_interaction_kind interaction) noexcept {
    return ((AkU64)method << 32) | (AkU64)interaction;
}

/// \brief Initialize an lsp_msg_hdr with default values
/// \param hdr Pointer to message header to initialize
static inline void lsp_msg_hdr_init(lsp_msg_hdr* hdr) noexcept {
    hdr->timestamp_nanos = 0;
    hdr->op_tag = 0;
    hdr->tag_id = 0;
    hdr->refcount.store(1, std::memory_order_relaxed);
}

/// \brief Initialize an lsp_msg_hdr with operation tag
/// \param hdr Pointer to message header to initialize
/// \param interaction Message interaction kind
/// \param method Message method type
/// \param tag_id Unique tag identifier for the message
static inline void lsp_msg_hdr_init_with_op(lsp_msg_hdr* hdr, lsp_interaction_kind interaction, lsp_method_type method, AkU64 tag_id) noexcept {
    hdr->timestamp_nanos = 0; // TODO: get actual timestamp
    hdr->op_tag = lsp_make_op_tag(method, interaction);
    hdr->tag_id = tag_id;
    hdr->refcount.store(1, std::memory_order_relaxed);
}

// ======================================================================================================================
// TEXT DOCUMENT IDENTIFIER UTILITIES
// ======================================================================================================================

/// \brief Initialize an lsp_text_document_identifier
/// \param id Pointer to identifier to initialize
/// \param uri Document URI
static inline void lsp_text_document_identifier_init(lsp_text_document_identifier* id, const lsp_uri* uri) noexcept {
    id->uri.chars = uri->chars;
}

// ======================================================================================================================
// TEXT DOCUMENT ITEM UTILITIES
// ======================================================================================================================

/// \brief Initialize an lsp_text_document_item
/// \param item Pointer to item to initialize
/// \param uri Document URI
/// \param language_id Language identifier
/// \param version Document version
/// \param text Document text content
static inline void lsp_text_document_item_init(lsp_text_document_item* item, const lsp_uri* uri, const lsp_string* language_id, int version, const lsp_string* text) noexcept {
    item->uri.chars = uri->chars;
    item->language_id = *language_id;
    item->version = version;
    item->text = *text;
}

// ======================================================================================================================
// VERSIONED TEXT DOCUMENT ID UTILITIES
// ======================================================================================================================

/// \brief Initialize an lsp_versioned_text_document_id
/// \param id Pointer to identifier to initialize
/// \param uri Document URI
/// \param version Document version
static inline void lsp_versioned_text_document_id_init(lsp_versioned_text_document_id* id, const lsp_uri* uri, int version) noexcept {
    id->uri.chars = uri->chars;
    id->version = version;
}

// ======================================================================================================================
// TEXT DOCUMENT POSITION PARAMS UTILITIES
// ======================================================================================================================

/// \brief Initialize an lsp_text_document_position_params
/// \param params Pointer to params to initialize
/// \param text_document Document identifier
/// \param position Position within the document
static inline void lsp_text_document_position_params_init(lsp_text_document_position_params* params, const lsp_text_document_identifier* text_document, const lsp_position* position) noexcept {
    params->text_document = *text_document;
    params->position = *position;
}

// ======================================================================================================================
// TEXT EDIT UTILITIES
// ======================================================================================================================

/// \brief Initialize an lsp_text_edit
/// \param edit Pointer to text edit to initialize
/// \param range Range to replace
/// \param new_text Text to insert
static inline void lsp_text_edit_init(lsp_text_edit* edit, const lsp_range* range, const lsp_string* new_text) noexcept {
    edit->range = *range;
    edit->new_text = *new_text;
}

// ======================================================================================================================
// OPTIONAL TYPE UTILITIES
// ======================================================================================================================

/// \brief Initialize an lsp_opt_string as NONE (absent)
/// \param opt Pointer to optional string to initialize
static inline void lsp_opt_string_init_none(lsp_opt_string* opt) noexcept {
    opt->kind = LSP_OPT_NONE;
}

/// \brief Initialize an lsp_opt_string as SOME (present)
/// \param opt Pointer to optional string to initialize
/// \param value String value
static inline void lsp_opt_string_init_some(lsp_opt_string* opt, const lsp_string* value) noexcept {
    opt->kind = LSP_OPT_SOME;
    opt->value = *value;
}

/// \brief Check if lsp_opt_string has a value
/// \param opt Pointer to optional string to check
/// \return true if value is present
static inline bool lsp_opt_string_is_some(const lsp_opt_string* opt) noexcept {
    return opt->kind == LSP_OPT_SOME;
}

/// \brief Get the value from an lsp_opt_string (unsafe)
/// \param opt Pointer to optional string
/// \return String value (only valid if lsp_opt_string_is_some returns true)
static inline const lsp_string* lsp_opt_string_get(const lsp_opt_string* opt) noexcept {
    return &opt->value;
}

// ======================================================================================================================
// DYNAMIC VALUE UTILITIES
// ======================================================================================================================


// ======================================================================================================================
// ERROR RESULT UTILITIES
// ======================================================================================================================

/// \brief Initialize an lsp_error_result
/// \param error Pointer to error result to initialize
/// \param code Error code
/// \param message Optional error message
static inline void lsp_error_result_init(lsp_error_result* error, int code, const lsp_opt_string* message) noexcept {
    error->code = code;
    error->message = *message;
}
