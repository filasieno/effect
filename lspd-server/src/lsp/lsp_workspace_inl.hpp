#pragma once

#include "lsp_workspace.hpp" // IWYU pragma: keep

/// \file lsp_workspace_inl.hpp
/// \brief Workspace feature message inline implementations

static inline struct lsp_workspace_symbol_request* lsp_init_workspace_symbol_request(void *mem, AkU64 tag_id) noexcept {
    struct lsp_workspace_symbol_request* req = (struct lsp_workspace_symbol_request*)mem;
    req->hdr.timestamp_nanos = ak_query_timer_ns();
    req->hdr.refcount.store(1, std::memory_order_relaxed);
    
    return req;
}

static inline struct lsp_workspace_symbol_response* lsp_init_workspace_symbol_response(void *mem, AkU64 tag_id) noexcept {
    struct lsp_workspace_symbol_response* resp = (struct lsp_workspace_symbol_response*)mem;
    resp->hdr.timestamp_nanos = ak_query_timer_ns();
    resp->hdr.refcount.store(1, std::memory_order_relaxed);
    
    return resp;
}

static inline struct lsp_workspace_symbol_error_result* lsp_init_workspace_symbol_error_result(void *mem, AkU64 tag_id) noexcept {
    struct lsp_workspace_symbol_error_result* resp = (struct lsp_workspace_symbol_error_result*)mem;
    resp->hdr.timestamp_nanos = ak_query_timer_ns();
    resp->hdr.refcount.store(1, std::memory_order_relaxed);
    
    
    return resp;
}

static inline struct lsp_workspace_symbol_resolve_request* lsp_init_workspace_symbol_resolve_request(void *mem, AkU64 tag_id) noexcept {
    struct lsp_workspace_symbol_resolve_request* req = (struct lsp_workspace_symbol_resolve_request*)mem;
    req->hdr.timestamp_nanos = ak_query_timer_ns();
    req->hdr.refcount.store(1, std::memory_order_relaxed);
    
    return req;
}

static inline struct lsp_workspace_symbol_resolve_response* lsp_init_workspace_symbol_resolve_response(void *mem, AkU64 tag_id) noexcept {
    struct lsp_workspace_symbol_resolve_response* resp = (struct lsp_workspace_symbol_resolve_response*)mem;
    resp->hdr.timestamp_nanos = ak_query_timer_ns();
    resp->hdr.refcount.store(1, std::memory_order_relaxed);
    
    return resp;
}

static inline struct lsp_workspace_symbol_resolve_error_result* lsp_init_workspace_symbol_resolve_error_result(void *mem, AkU64 tag_id) noexcept {
    struct lsp_workspace_symbol_resolve_error_result* resp = (struct lsp_workspace_symbol_resolve_error_result*)mem;
    resp->hdr.timestamp_nanos = ak_query_timer_ns();
    resp->hdr.refcount.store(1, std::memory_order_relaxed);
    // resp->hdr.tag_id = tag_id;
    // resp->hdr.interaction_kind = LSP_INTERACT_ERROR_RESULT;
    // resp->hdr.method_type = LSP_METHOD_WORKSPACE_SYMBOL_RESOLVE;
    // resp->hdr.op_tag = LSP_OP_NONE;
    return resp;
}

static inline struct lsp_workspace_execute_command_request* lsp_init_workspace_execute_command_request(void *mem, AkU64 tag_id) noexcept {
    struct lsp_workspace_execute_command_request* req = (struct lsp_workspace_execute_command_request*)mem;
    req->hdr.timestamp_nanos = ak_query_timer_ns();
    req->hdr.refcount.store(1, std::memory_order_relaxed);
    // req->hdr.tag_id = tag_id;
    // req->hdr.interaction_kind = LSP_INTERACT_REQUEST;
    // req->hdr.method_type = LSP_METHOD_WORKSPACE_EXECUTE_COMMAND;
    // req->hdr.op_tag = LSP_OP_NONE;
    return req;
}

static inline struct lsp_workspace_execute_command_response* lsp_init_workspace_execute_command_response(void *mem, AkU64 tag_id) noexcept {
    struct lsp_workspace_execute_command_response* resp = (struct lsp_workspace_execute_command_response*)mem;
    resp->hdr.timestamp_nanos = ak_query_timer_ns();
    resp->hdr.refcount.store(1, std::memory_order_relaxed);
    // resp->hdr.tag_id = tag_id;
    // resp->hdr.interaction_kind = LSP_INTERACT_RESPONSE;
    // resp->hdr.method_type = LSP_METHOD_WORKSPACE_EXECUTE_COMMAND;
    // resp->hdr.op_tag = LSP_OP_NONE;
    return resp;
}

static inline struct lsp_workspace_execute_command_error_result* lsp_init_workspace_execute_command_error_result(void *mem, AkU64 tag_id) noexcept {
    struct lsp_workspace_execute_command_error_result* resp = (struct lsp_workspace_execute_command_error_result*)mem;
    resp->hdr.timestamp_nanos = ak_query_timer_ns();
    resp->hdr.refcount.store(1, std::memory_order_relaxed);
    // resp->hdr.tag_id = tag_id;
    // resp->hdr.interaction_kind = LSP_INTERACT_ERROR_RESULT;
    // resp->hdr.method_type = LSP_METHOD_WORKSPACE_EXECUTE_COMMAND;
    // resp->hdr.op_tag = LSP_OP_NONE;
    return resp;
}

static inline struct lsp_workspace_apply_edit_request* lsp_init_workspace_apply_edit_request(void *mem, AkU64 tag_id) noexcept {
    struct lsp_workspace_apply_edit_request* req = (struct lsp_workspace_apply_edit_request*)mem;
    req->hdr.timestamp_nanos = ak_query_timer_ns();
    req->hdr.refcount.store(1, std::memory_order_relaxed);
    // req->hdr.tag_id = tag_id;
    // req->hdr.interaction_kind = LSP_INTERACT_REQUEST;
    // req->hdr.method_type = LSP_METHOD_WORKSPACE_APPLY_EDIT;
    // req->hdr.op_tag = LSP_OP_NONE;
    return req;
}

static inline struct lsp_workspace_apply_edit_response* lsp_init_workspace_apply_edit_response(void *mem, AkU64 tag_id) noexcept {
    struct lsp_workspace_apply_edit_response* resp = (struct lsp_workspace_apply_edit_response*)mem;
    resp->hdr.timestamp_nanos = ak_query_timer_ns();
    resp->hdr.refcount.store(1, std::memory_order_relaxed);
    // resp->hdr.tag_id = tag_id;
    // resp->hdr.interaction_kind = LSP_INTERACT_RESPONSE;
    // resp->hdr.method_type = LSP_METHOD_WORKSPACE_APPLY_EDIT;
    // resp->hdr.op_tag = LSP_OP_NONE;
    return resp;
}

static inline struct lsp_workspace_apply_edit_error_result* lsp_init_workspace_apply_edit_error_result(void *mem, AkU64 tag_id) noexcept {
    struct lsp_workspace_apply_edit_error_result* resp = (struct lsp_workspace_apply_edit_error_result*)mem;
    resp->hdr.timestamp_nanos = ak_query_timer_ns();
    resp->hdr.refcount.store(1, std::memory_order_relaxed);
    // resp->hdr.tag_id = tag_id;
    // resp->hdr.interaction_kind = LSP_INTERACT_ERROR_RESULT;
    // resp->hdr.method_type = LSP_METHOD_WORKSPACE_APPLY_EDIT;
    // resp->hdr.op_tag = LSP_OP_NONE;
    return resp;
}

static inline struct lsp_workspace_did_change_workspace_folders_notification* lsp_init_workspace_did_change_workspace_folders_notification(void *mem, AkU64 tag_id) noexcept {
    struct lsp_workspace_did_change_workspace_folders_notification* notif = (struct lsp_workspace_did_change_workspace_folders_notification*)mem;
    notif->hdr.timestamp_nanos = ak_query_timer_ns();
    notif->hdr.refcount.store(1, std::memory_order_relaxed);
    // notif->hdr.tag_id = tag_id;
    // notif->hdr.interaction_kind = LSP_INTERACT_NOTIFICATION;
    // notif->hdr.method_type = LSP_METHOD_WORKSPACE_DID_CHANGE_WORKSPACE_FOLDERS;
    // notif->hdr.op_tag = LSP_OP_NONE;
    return notif;
}

static inline struct lsp_workspace_configuration_request* lsp_init_workspace_configuration_request(void *mem, AkU64 tag_id) noexcept {
    struct lsp_workspace_configuration_request* req = (struct lsp_workspace_configuration_request*)mem;
    req->hdr.timestamp_nanos = ak_query_timer_ns();
    req->hdr.refcount.store(1, std::memory_order_relaxed);
    // req->hdr.tag_id = tag_id;
    // req->hdr.interaction_kind = LSP_INTERACT_REQUEST;
    // req->hdr.method_type = LSP_METHOD_WORKSPACE_CONFIGURATION;
    // req->hdr.op_tag = LSP_OP_NONE;
    return req;
}

static inline struct lsp_workspace_configuration_response* lsp_init_workspace_configuration_response(void *mem, AkU64 tag_id) noexcept {
    struct lsp_workspace_configuration_response* resp = (struct lsp_workspace_configuration_response*)mem;
    resp->hdr.timestamp_nanos = ak_query_timer_ns();
    resp->hdr.refcount.store(1, std::memory_order_relaxed);
    // resp->hdr.tag_id = tag_id;
    // resp->hdr.interaction_kind = LSP_INTERACT_RESPONSE;
    // resp->hdr.method_type = LSP_METHOD_WORKSPACE_CONFIGURATION;
    // resp->hdr.op_tag = LSP_OP_NONE;
    return resp;
}

static inline struct lsp_workspace_configuration_error_result* lsp_init_workspace_configuration_error_result(void *mem, AkU64 tag_id) noexcept {
    struct lsp_workspace_configuration_error_result* resp = (struct lsp_workspace_configuration_error_result*)mem;
    resp->hdr.timestamp_nanos = ak_query_timer_ns();
    resp->hdr.refcount.store(1, std::memory_order_relaxed);
    // resp->hdr.tag_id = tag_id;
    // resp->hdr.interaction_kind = LSP_INTERACT_ERROR_RESULT;
    // resp->hdr.method_type = LSP_METHOD_WORKSPACE_CONFIGURATION;
    // resp->hdr.op_tag = LSP_OP_NONE;
    return resp;
}

static inline struct lsp_workspace_folders_request* lsp_init_workspace_folders_request(void *mem, AkU64 tag_id) noexcept {
    struct lsp_workspace_folders_request* req = (struct lsp_workspace_folders_request*)mem;
    req->hdr.timestamp_nanos = ak_query_timer_ns();
    req->hdr.refcount.store(1, std::memory_order_relaxed);
    // req->hdr.tag_id = tag_id;
    // req->hdr.interaction_kind = LSP_INTERACT_REQUEST;
    // req->hdr.method_type = LSP_METHOD_WORKSPACE_WORKSPACE_FOLDERS;
    // req->hdr.op_tag = LSP_OP_NONE;
    return req;
}

static inline struct lsp_workspace_folders_partial_response* lsp_init_workspace_folders_partial_response(void *mem, AkU64 tag_id) noexcept {
    struct lsp_workspace_folders_partial_response* resp = (struct lsp_workspace_folders_partial_response*)mem;
    resp->hdr.timestamp_nanos = ak_query_timer_ns();
    resp->hdr.refcount.store(1, std::memory_order_relaxed);
    // resp->hdr.tag_id = tag_id;
    // resp->hdr.interaction_kind = LSP_INTERACT_RESPONSE;
    // resp->hdr.method_type = LSP_METHOD_WORKSPACE_WORKSPACE_FOLDERS;
    // resp->hdr.op_tag = LSP_OP_NONE;
    return resp;
}

static inline struct lsp_workspace_folders_response* lsp_init_workspace_folders_response(void *mem, AkU64 tag_id) noexcept {
    struct lsp_workspace_folders_response* resp = (struct lsp_workspace_folders_response*)mem;
    resp->hdr.timestamp_nanos = ak_query_timer_ns();
    resp->hdr.refcount.store(1, std::memory_order_relaxed);
    // resp->hdr.tag_id = tag_id;
    // resp->hdr.interaction_kind = LSP_INTERACT_RESPONSE;
    // resp->hdr.method_type = LSP_METHOD_WORKSPACE_WORKSPACE_FOLDERS;
    // resp->hdr.op_tag = LSP_OP_NONE;
    return resp;
}

static inline struct lsp_workspace_folders_error_result* lsp_init_workspace_folders_error_result(void *mem, AkU64 tag_id) noexcept {
    struct lsp_workspace_folders_error_result* resp = (struct lsp_workspace_folders_error_result*)mem;
    resp->hdr.timestamp_nanos = ak_query_timer_ns();
    resp->hdr.refcount.store(1, std::memory_order_relaxed);
    // resp->hdr.tag_id = tag_id;
    // resp->hdr.interaction_kind = LSP_INTERACT_ERROR_RESULT;
    // resp->hdr.method_type = LSP_METHOD_WORKSPACE_WORKSPACE_FOLDERS;
    // resp->hdr.op_tag = LSP_OP_NONE;
    return resp;
}


// static inline struct lsp_text_document_did_open_notification* lsp_init_text_document_did_open_notification(void* mem, AkU64 tag_id) noexcept {
//     auto* m = (struct lsp_text_document_did_open_notification*)mem;
//     lsp_msg_hdr_init_with_op(&m->hdr, LSP_INTERACT_NOTIFICATION, LSP_METHOD_TEXT_DOCUMENT_DID_OPEN, tag_id);
//     return m;
// }
// static inline struct lsp_text_document_did_change_notification* lsp_init_text_document_did_change_notification(void* mem, AkU64 tag_id) noexcept {
//     auto* m = (struct lsp_text_document_did_change_notification*)mem;
//     lsp_msg_hdr_init_with_op(&m->hdr, LSP_INTERACT_NOTIFICATION, LSP_METHOD_TEXT_DOCUMENT_DID_CHANGE, tag_id);
//     return m;
// }
// static inline struct lsp_text_document_did_close_notification* lsp_init_text_document_did_close_notification(void* mem, AkU64 tag_id) noexcept {
//     auto* m = (struct lsp_text_document_did_close_notification*)mem;
//     lsp_msg_hdr_init_with_op(&m->hdr, LSP_INTERACT_NOTIFICATION, LSP_METHOD_TEXT_DOCUMENT_DID_CLOSE, tag_id);
//     return m;
// }
// static inline struct lsp_text_document_did_save_notification* lsp_init_text_document_did_save_notification(void* mem, AkU64 tag_id) noexcept {
//     auto* m = (struct lsp_text_document_did_save_notification*)mem;
//     lsp_msg_hdr_init_with_op(&m->hdr, LSP_INTERACT_NOTIFICATION, LSP_METHOD_TEXT_DOCUMENT_DID_SAVE, tag_id);
//     m->text.kind = LSP_OPT_NONE;
//     return m;
// }
// static inline struct lsp_text_document_will_save_notification* lsp_init_text_document_will_save_notification(void* mem, AkU64 tag_id) noexcept {
//     auto* m = (struct lsp_text_document_will_save_notification*)mem;
//     lsp_msg_hdr_init_with_op(&m->hdr, LSP_INTERACT_NOTIFICATION, LSP_METHOD_TEXT_DOCUMENT_WILL_SAVE, tag_id);
//     return m;
// }

// // text_document/will_save_wait_until/request, partial_response, response, error_result

// static inline struct lsp_text_document_will_save_wait_until_request* lsp_init_text_document_will_save_wait_until_request(void* mem, AkU64 tag_id) noexcept {
//     auto* m = (struct lsp_text_document_will_save_wait_until_request*)mem;
//     lsp_msg_hdr_init_with_op(&m->hdr, LSP_INTERACT_REQUEST, LSP_METHOD_TEXT_DOCUMENT_WILL_SAVE_WAIT_UNTIL, tag_id);
//     return m;
// }
// static inline struct lsp_text_document_will_save_wait_until_partial_response* lsp_init_text_document_will_save_wait_until_partial_response(void* mem, AkU64 tag_id) noexcept {
//     auto* m = (struct lsp_text_document_will_save_wait_until_partial_response*)mem;
//     lsp_msg_hdr_init_with_op(&m->hdr, LSP_INTERACT_PARTIAL_RESPONSE, LSP_METHOD_TEXT_DOCUMENT_WILL_SAVE_WAIT_UNTIL, tag_id);
//     m->partial.kind = LSP_OPT_NONE;
//     m->partial.head = nullptr;
//     m->partial.count = 0;
//     return m;
// }
// static inline struct lsp_text_document_will_save_wait_until_response* lsp_init_text_document_will_save_wait_until_response(void* mem, AkU64 tag_id) noexcept {
//     auto* m = (struct lsp_text_document_will_save_wait_until_response*)mem;
//     lsp_msg_hdr_init_with_op(&m->hdr, LSP_INTERACT_RESPONSE, LSP_METHOD_TEXT_DOCUMENT_WILL_SAVE_WAIT_UNTIL, tag_id);
//     m->result.kind = LSP_OPT_NONE;
//     m->result.head = nullptr;
//     m->result.count = 0;
//     return m;
// }
// static inline struct lsp_text_document_will_save_wait_until_error_result* lsp_init_text_document_will_save_wait_until_error_result(void* mem, AkU64 tag_id) noexcept {
//     auto* m = (struct lsp_text_document_will_save_wait_until_error_result*)mem;
//     lsp_msg_hdr_init_with_op(&m->hdr, LSP_INTERACT_ERROR_RESULT, LSP_METHOD_TEXT_DOCUMENT_WILL_SAVE_WAIT_UNTIL, tag_id);
//     return m;
// }
