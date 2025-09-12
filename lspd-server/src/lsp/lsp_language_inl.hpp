#pragma once

#include "lsp_language.hpp" // IWYU pragma: keep

// Inline initializers for language features

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


