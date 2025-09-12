#pragma once

#include "lsp_language.hpp" // IWYU pragma: keep

// Inline initializers for language features

static inline struct lsp_text_document_diagnostic_request* lsp_init_text_document_diagnostic_request(void* mem, AkU64 tag_id) noexcept {
    struct lsp_text_document_diagnostic_request* m = (struct lsp_text_document_diagnostic_request*)mem;
    m->hdr.timestamp_nanos = ak_query_timer_ns();
    m->hdr.refcount.store(1, std::memory_order_relaxed);
    return m;
}
static inline struct lsp_text_document_diagnostic_response* lsp_init_text_document_diagnostic_response(void* mem, AkU64 tag_id) noexcept {
    struct lsp_text_document_diagnostic_response* m = (struct lsp_text_document_diagnostic_response*)mem;
    m->hdr.timestamp_nanos = ak_query_timer_ns();
    m->hdr.refcount.store(1, std::memory_order_relaxed);
    return m;
}
static inline struct lsp_text_document_diagnostic_error_result* lsp_init_text_document_diagnostic_error_result(void* mem, AkU64 tag_id) noexcept {
    struct lsp_text_document_diagnostic_error_result* m = (struct lsp_text_document_diagnostic_error_result*)mem;
    m->hdr.timestamp_nanos = ak_query_timer_ns();
    m->hdr.refcount.store(1, std::memory_order_relaxed);
    return m;
}

static inline struct lsp_text_document_publish_diagnostics_notification* lsp_init_text_document_publish_diagnostics_notification(void* mem, AkU64 tag_id) noexcept {
    struct lsp_text_document_publish_diagnostics_notification* m = (struct lsp_text_document_publish_diagnostics_notification*)mem;
    m->hdr.timestamp_nanos = ak_query_timer_ns();
    m->hdr.refcount.store(1, std::memory_order_relaxed);
    return m;
}

// static inline struct lsp_text_document_definition_request* lsp_init_text_document_definition_request(void* mem, AkU64 tag_id) noexcept {
//     auto* m = (struct lsp_text_document_definition_request*)mem;
//     lsp_msg_hdr_init_with_op(&m->hdr, LSP_INTERACT_REQUEST, LSP_METHOD_TEXT_DOCUMENT_DEFINITION, tag_id);
//     return m;
// }
// static inline struct lsp_text_document_definition_partial_response* lsp_init_text_document_definition_partial_response(void* mem, AkU64 tag_id) noexcept {
//     auto* m = (struct lsp_text_document_definition_partial_response*)mem;
//     lsp_msg_hdr_init_with_op(&m->hdr, LSP_INTERACT_PARTIAL_RESPONSE, LSP_METHOD_TEXT_DOCUMENT_DEFINITION, tag_id);
//     return m;
// }
// static inline struct lsp_text_document_definition_response* lsp_init_text_document_definition_response(void* mem, AkU64 tag_id) noexcept {
//     auto* m = (struct lsp_text_document_definition_response*)mem;
//     lsp_msg_hdr_init_with_op(&m->hdr, LSP_INTERACT_RESPONSE, LSP_METHOD_TEXT_DOCUMENT_DEFINITION, tag_id);
//     m->result.kind = LSP_DEF_NONE;
//     return m;
// }
// static inline struct lsp_text_document_definition_error_result* lsp_init_text_document_definition_error_result(void* mem, AkU64 tag_id) noexcept {
//     auto* m = (struct lsp_text_document_definition_error_result*)mem;
//     lsp_msg_hdr_init_with_op(&m->hdr, LSP_INTERACT_ERROR_RESULT, LSP_METHOD_TEXT_DOCUMENT_DEFINITION, tag_id);
//     return m;
// }


/// \brief Initialize an lsp_text_document_position_params
/// \param params Pointer to params to initialize
/// \param text_document Document identifier
/// \param position Position within the document
static inline void lsp_text_document_position_params_init(lsp_text_document_position_params* params, const lsp_text_document_identifier* text_document, const lsp_position* position) noexcept {
    params->text_document = *text_document;
    params->position = *position;
}


