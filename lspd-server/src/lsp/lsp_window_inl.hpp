#pragma once

#include "lsp_window.hpp" // IWYU pragma: keep

/// \file lsp_window_inl.hpp
/// \brief Window and UI message inline implementations

static inline struct lsp_window_show_document_request* lsp_init_window_show_document_request(void *mem, AkU64 tag_id) noexcept {
    struct lsp_window_show_document_request* req = (struct lsp_window_show_document_request*)mem;
    req->hdr.timestamp_nanos = ak_query_timer_ns();
    req->hdr.refcount.store(1, std::memory_order_relaxed);
    req->hdr.tag_id = tag_id;
    req->hdr.interaction_kind = LSP_INTERACT_REQUEST;
    req->hdr.method_type = LSP_METHOD_WINDOW_SHOW_DOCUMENT;
    req->hdr.op_tag = LSP_OP_NONE;
    return req;
}

static inline struct lsp_window_show_document_partial_response* lsp_init_window_show_document_partial_response(void *mem, AkU64 tag_id) noexcept {
    struct lsp_window_show_document_partial_response* resp = (struct lsp_window_show_document_partial_response*)mem;
    resp->hdr.timestamp_nanos = ak_query_timer_ns();
    resp->hdr.refcount.store(1, std::memory_order_relaxed);
    resp->hdr.tag_id = tag_id;
    resp->hdr.interaction_kind = LSP_INTERACT_RESPONSE;
    resp->hdr.method_type = LSP_METHOD_WINDOW_SHOW_DOCUMENT;
    resp->hdr.op_tag = LSP_OP_NONE;
    return resp;
}

static inline struct lsp_window_show_document_response* lsp_init_window_show_document_response(void *mem, AkU64 tag_id) noexcept {
    struct lsp_window_show_document_response* resp = (struct lsp_window_show_document_response*)mem;
    resp->hdr.timestamp_nanos = ak_query_timer_ns();
    resp->hdr.refcount.store(1, std::memory_order_relaxed);
    resp->hdr.tag_id = tag_id;
    resp->hdr.interaction_kind = LSP_INTERACT_RESPONSE;
    resp->hdr.method_type = LSP_METHOD_WINDOW_SHOW_DOCUMENT;
    resp->hdr.op_tag = LSP_OP_NONE;
    return resp;
}

static inline struct lsp_window_show_document_error_result* lsp_init_window_show_document_error_result(void *mem, AkU64 tag_id) noexcept {
    struct lsp_window_show_document_error_result* resp = (struct lsp_window_show_document_error_result*)mem;
    resp->hdr.timestamp_nanos = ak_query_timer_ns();
    resp->hdr.refcount.store(1, std::memory_order_relaxed);
    resp->hdr.tag_id = tag_id;
    resp->hdr.interaction_kind = LSP_INTERACT_ERROR_RESULT;
    resp->hdr.method_type = LSP_METHOD_WINDOW_SHOW_DOCUMENT;
    resp->hdr.op_tag = LSP_OP_NONE;
    return resp;
}

static inline struct lsp_window_show_message_notification* lsp_init_window_show_message_notification(void *mem, AkU64 tag_id) noexcept {
    struct lsp_window_show_message_notification* notif = (struct lsp_window_show_message_notification*)mem;
    notif->hdr.timestamp_nanos = ak_query_timer_ns();
    notif->hdr.refcount.store(1, std::memory_order_relaxed);
    notif->hdr.tag_id = tag_id;
    notif->hdr.interaction_kind = LSP_INTERACT_NOTIFICATION;
    notif->hdr.method_type = LSP_METHOD_WINDOW_SHOW_MESSAGE;
    notif->hdr.op_tag = LSP_OP_NONE;
    return notif;
}

static inline struct lsp_window_show_message_request* lsp_init_window_show_message_request(void *mem, AkU64 tag_id) noexcept {
    struct lsp_window_show_message_request* req = (struct lsp_window_show_message_request*)mem;
    req->hdr.timestamp_nanos = ak_query_timer_ns();
    req->hdr.refcount.store(1, std::memory_order_relaxed);
    req->hdr.tag_id = tag_id;
    req->hdr.interaction_kind = LSP_INTERACT_REQUEST;
    req->hdr.method_type = LSP_METHOD_WINDOW_SHOW_MESSAGE_REQUEST;
    req->hdr.op_tag = LSP_OP_NONE;
    return req;
}

static inline struct lsp_window_show_message_partial_response* lsp_init_window_show_message_partial_response(void *mem, AkU64 tag_id) noexcept {
    struct lsp_window_show_message_partial_response* resp = (struct lsp_window_show_message_partial_response*)mem;
    resp->hdr.timestamp_nanos = ak_query_timer_ns();
    resp->hdr.refcount.store(1, std::memory_order_relaxed);
    resp->hdr.tag_id = tag_id;
    resp->hdr.interaction_kind = LSP_INTERACT_RESPONSE;
    resp->hdr.method_type = LSP_METHOD_WINDOW_SHOW_MESSAGE_REQUEST;
    resp->hdr.op_tag = LSP_OP_NONE;
    return resp;
}

static inline struct lsp_window_show_message_response* lsp_init_window_show_message_response(void *mem, AkU64 tag_id) noexcept {
    struct lsp_window_show_message_response* resp = (struct lsp_window_show_message_response*)mem;
    resp->hdr.timestamp_nanos = ak_query_timer_ns();
    resp->hdr.refcount.store(1, std::memory_order_relaxed);
    resp->hdr.tag_id = tag_id;
    resp->hdr.interaction_kind = LSP_INTERACT_RESPONSE;
    resp->hdr.method_type = LSP_METHOD_WINDOW_SHOW_MESSAGE_REQUEST;
    resp->hdr.op_tag = LSP_OP_NONE;
    return resp;
}

static inline struct lsp_window_show_message_error_result* lsp_init_window_show_message_error_result(void *mem, AkU64 tag_id) noexcept {
    struct lsp_window_show_message_error_result* resp = (struct lsp_window_show_message_error_result*)mem;
    resp->hdr.timestamp_nanos = ak_query_timer_ns();
    resp->hdr.refcount.store(1, std::memory_order_relaxed);
    resp->hdr.tag_id = tag_id;
    resp->hdr.interaction_kind = LSP_INTERACT_ERROR_RESULT;
    resp->hdr.method_type = LSP_METHOD_WINDOW_SHOW_MESSAGE_REQUEST;
    resp->hdr.op_tag = LSP_OP_NONE;
    return resp;
}
