#pragma once

#include "lsp_window.hpp" // IWYU pragma: keep


static inline struct lsp_window_show_document_request* lsp_init_window_show_document_request(void *mem, AkU64 tag_id) noexcept {
    struct lsp_window_show_document_request* req = (struct lsp_window_show_document_request*)mem;
    req->hdr.timestamp_nanos = ak_query_timer_ns();
    req->hdr.refcount.store(1, std::memory_order_relaxed);
    req->hdr.tag_id = tag_id;
    // TODO: set interaction kind and method type
    return req;
}

static inline struct lsp_window_show_document_partial_response* lsp_init_window_show_document_partial_response(void *mem, AkU64 tag_id) noexcept {
    struct lsp_window_show_document_partial_response* resp = (struct lsp_window_show_document_partial_response*)mem;
    resp->hdr.timestamp_nanos = ak_query_timer_ns();
    resp->hdr.refcount.store(1, std::memory_order_relaxed);
    resp->hdr.tag_id = tag_id;
    // TODO: set interaction kind and method type
    return resp;
}

static inline struct lsp_window_show_document_response* lsp_init_window_show_document_response(void *mem, AkU64 tag_id) noexcept {
    struct lsp_window_show_document_response* resp = (struct lsp_window_show_document_response*)mem;
    resp->hdr.timestamp_nanos = ak_query_timer_ns();
    resp->hdr.refcount.store(1, std::memory_order_relaxed);
    resp->hdr.tag_id = tag_id;
    // TODO: set interaction kind and method type

    return resp;
}

static inline struct lsp_window_show_document_error_result* lsp_init_window_show_document_error_result(void *mem, AkU64 tag_id) noexcept {
    struct lsp_window_show_document_error_result* resp = (struct lsp_window_show_document_error_result*)mem;
    resp->hdr.timestamp_nanos = ak_query_timer_ns();
    resp->hdr.refcount.store(1, std::memory_order_relaxed);
    resp->hdr.tag_id = tag_id;
    // TODO: set interaction kind and method type

    return resp;
}

static inline struct lsp_window_show_message_notification* lsp_init_window_show_message_notification(void *mem, AkU64 tag_id) noexcept {
    struct lsp_window_show_message_notification* notif = (struct lsp_window_show_message_notification*)mem;
    notif->hdr.timestamp_nanos = ak_query_timer_ns();
    notif->hdr.refcount.store(1, std::memory_order_relaxed);
    notif->hdr.tag_id = tag_id;
    // TODO: set interaction kind and method type

    return notif;
}

static inline struct lsp_window_show_message_request* lsp_init_window_show_message_request(void *mem, AkU64 tag_id) noexcept {
    struct lsp_window_show_message_request* req = (struct lsp_window_show_message_request*)mem;
    req->hdr.timestamp_nanos = ak_query_timer_ns();
    req->hdr.refcount.store(1, std::memory_order_relaxed);
    req->hdr.tag_id = tag_id;
    // TODO: set interaction kind and method type

    return req;
}

static inline struct lsp_window_show_message_partial_response* lsp_init_window_show_message_partial_response(void *mem, AkU64 tag_id) noexcept {
    struct lsp_window_show_message_partial_response* resp = (struct lsp_window_show_message_partial_response*)mem;
    resp->hdr.timestamp_nanos = ak_query_timer_ns();
    resp->hdr.refcount.store(1, std::memory_order_relaxed);
    resp->hdr.tag_id = tag_id;
    // TODO: set interaction kind and method type

    return resp;
}

static inline struct lsp_window_show_message_response* lsp_init_window_show_message_response(void *mem, AkU64 tag_id) noexcept {
    struct lsp_window_show_message_response* resp = (struct lsp_window_show_message_response*)mem;
    resp->hdr.timestamp_nanos = ak_query_timer_ns();
    resp->hdr.refcount.store(1, std::memory_order_relaxed);
    resp->hdr.tag_id = tag_id;
    // TODO: set interaction kind and method type

    return resp;
}

static inline struct lsp_window_show_message_error_result* lsp_init_window_show_message_error_result(void *mem, AkU64 tag_id) noexcept {
    struct lsp_window_show_message_error_result* resp = (struct lsp_window_show_message_error_result*)mem;
    resp->hdr.timestamp_nanos = ak_query_timer_ns();
    resp->hdr.refcount.store(1, std::memory_order_relaxed);
    resp->hdr.tag_id = tag_id;
    // TODO: set interaction kind and method type

    return resp;
}

static inline struct lsp_window_log_message_notification* lsp_init_window_log_message_notification(void *mem, AkU64 tag_id) noexcept {
    struct lsp_window_log_message_notification* notif = (struct lsp_window_log_message_notification*)mem;
    notif->hdr.timestamp_nanos = ak_query_timer_ns();
    notif->hdr.refcount.store(1, std::memory_order_relaxed);
    notif->hdr.tag_id = tag_id;
    return notif;
}

static inline struct lsp_window_work_done_progress_create_request* lsp_init_window_work_done_progress_create_request(void *mem, AkU64 tag_id) noexcept {
    struct lsp_window_work_done_progress_create_request* req = (struct lsp_window_work_done_progress_create_request*)mem;
    req->hdr.timestamp_nanos = ak_query_timer_ns();
    req->hdr.refcount.store(1, std::memory_order_relaxed);
    req->hdr.tag_id = tag_id;
    return req;
}
static inline struct lsp_window_work_done_progress_create_response* lsp_init_window_work_done_progress_create_response(void *mem, AkU64 tag_id) noexcept {
    struct lsp_window_work_done_progress_create_response* resp = (struct lsp_window_work_done_progress_create_response*)mem;
    resp->hdr.timestamp_nanos = ak_query_timer_ns();
    resp->hdr.refcount.store(1, std::memory_order_relaxed);
    resp->hdr.tag_id = tag_id;
    return resp;
}
static inline struct lsp_window_work_done_progress_create_error_result* lsp_init_window_work_done_progress_create_error_result(void *mem, AkU64 tag_id) noexcept {
    struct lsp_window_work_done_progress_create_error_result* resp = (struct lsp_window_work_done_progress_create_error_result*)mem;
    resp->hdr.timestamp_nanos = ak_query_timer_ns();
    resp->hdr.refcount.store(1, std::memory_order_relaxed);
    resp->hdr.tag_id = tag_id;
    return resp;
}

static inline struct lsp_window_work_done_progress_cancel_request* lsp_init_window_work_done_progress_cancel_request(void *mem, AkU64 tag_id) noexcept {
    struct lsp_window_work_done_progress_cancel_request* req = (struct lsp_window_work_done_progress_cancel_request*)mem;
    req->hdr.timestamp_nanos = ak_query_timer_ns();
    req->hdr.refcount.store(1, std::memory_order_relaxed);
    req->hdr.tag_id = tag_id;
    return req;
}
static inline struct lsp_window_work_done_progress_cancel_response* lsp_init_window_work_done_progress_cancel_response(void *mem, AkU64 tag_id) noexcept {
    struct lsp_window_work_done_progress_cancel_response* resp = (struct lsp_window_work_done_progress_cancel_response*)mem;
    resp->hdr.timestamp_nanos = ak_query_timer_ns();
    resp->hdr.refcount.store(1, std::memory_order_relaxed);
    resp->hdr.tag_id = tag_id;
    return resp;
}
static inline struct lsp_window_work_done_progress_cancel_error_result* lsp_init_window_work_done_progress_cancel_error_result(void *mem, AkU64 tag_id) noexcept {
    struct lsp_window_work_done_progress_cancel_error_result* resp = (struct lsp_window_work_done_progress_cancel_error_result*)mem;
    resp->hdr.timestamp_nanos = ak_query_timer_ns();
    resp->hdr.refcount.store(1, std::memory_order_relaxed);
    resp->hdr.tag_id = tag_id;
    return resp;
}
