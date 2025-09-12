#pragma once

#include "lsp_doc_sync.hpp"

/// \file lsp_doc_sync_inl.hpp
/// \brief Inline implementations for document synchronization LSP types
///
/// This file contains inline function implementations for the LSP document synchronization types
/// defined in lsp_doc_sync.hpp.

// ======================================================================================================================
// TEXT DOCUMENT IDENTIFIER UTILITIES
// ======================================================================================================================

/// \brief Initialize an lsp_text_document_id structure
/// \param id Pointer to document ID to initialize
/// \param uri Document URI
/// \param version Document version
static inline void lsp_text_document_id_init(lsp_text_document_id* id, const lsp_uri* uri, int version) noexcept {
    id->uri.chars = uri->chars;
    id->version = version;
}

// ======================================================================================================================
// TEXT DOCUMENT UPDATE NOTIFICATION UTILITIES
// ======================================================================================================================

/// \brief Initialize an lsp_text_document_update_notification structure
/// \param notif Pointer to notification to initialize
/// \param id Document ID
/// \param range Range to update
/// \param text New text content
static inline void lsp_text_document_update_notification_init(lsp_text_document_update_notification* notif, const lsp_text_document_id* id, const lsp_range* range, const lsp_string* text) noexcept {
    notif->id = *id;
    notif->range = *range;
    notif->text = *text;
}

// ======================================================================================================================
// TEXT DOCUMENT DID OPEN NOTIFICATION UTILITIES
// ======================================================================================================================

/// \brief Initialize an lsp_text_document_did_open_notification structure
/// \param notif Pointer to notification to initialize
/// \param text_document Document information
static inline void lsp_text_document_did_open_notification_init(lsp_text_document_did_open_notification* notif, const lsp_text_document_item* text_document) noexcept {
    notif->text_document = *text_document;
}

// ======================================================================================================================
// TEXT DOCUMENT CONTENT CHANGE UTILITIES
// ======================================================================================================================

/// \brief Initialize an lsp_text_document_content_change structure
/// \param change Pointer to content change to initialize
/// \param range Range to replace
/// \param text Replacement text
static inline void lsp_text_document_content_change_init(lsp_text_document_content_change* change, const lsp_range* range, const lsp_string* text) noexcept {
    change->range = *range;
    change->text = *text;
}

// ======================================================================================================================
// TEXT DOCUMENT DID CHANGE NOTIFICATION UTILITIES
// ======================================================================================================================

/// \brief Initialize an lsp_text_document_did_change_notification structure
/// \param notif Pointer to notification to initialize
/// \param text_document Versioned document ID
/// \param changes Linked list of content changes
static inline void lsp_text_document_did_change_notification_init(lsp_text_document_did_change_notification* notif, const lsp_versioned_text_document_id* text_document, struct lsp_list<lsp_text_document_content_change>* changes) noexcept {
    notif->text_document = *text_document;
    notif->changes = changes;
}

// ======================================================================================================================
// TEXT DOCUMENT DID CLOSE NOTIFICATION UTILITIES
// ======================================================================================================================

/// \brief Initialize an lsp_text_document_did_close_notification structure
/// \param notif Pointer to notification to initialize
/// \param uri Document URI
static inline void lsp_text_document_did_close_notification_init(lsp_text_document_did_close_notification* notif, const lsp_uri* uri) noexcept {
    notif->uri.chars = uri->chars;
}

// ======================================================================================================================
// TEXT DOCUMENT DID SAVE NOTIFICATION UTILITIES
// ======================================================================================================================

/// \brief Initialize an lsp_text_document_did_save_notification structure
/// \param notif Pointer to notification to initialize
/// \param uri Document URI
/// \param text Optional saved text content
static inline void lsp_text_document_did_save_notification_init(lsp_text_document_did_save_notification* notif, const lsp_uri* uri, const lsp_opt_string* text) noexcept {
    notif->uri.chars = uri->chars;
    notif->text = *text;
}

// ======================================================================================================================
// TEXT DOCUMENT WILL SAVE NOTIFICATION UTILITIES
// ======================================================================================================================

/// \brief Initialize an lsp_text_document_will_save_notification structure
/// \param notif Pointer to notification to initialize
/// \param uri Document URI
/// \param reason Save reason
static inline void lsp_text_document_will_save_notification_init(lsp_text_document_will_save_notification* notif, const lsp_uri* uri, int reason) noexcept {
    notif->uri.chars = uri->chars;
    notif->reason = reason;
}

// ======================================================================================================================
// TEXT DOCUMENT WILL SAVE WAIT UNTIL REQUEST UTILITIES
// ======================================================================================================================

/// \brief Initialize an lsp_text_document_will_save_wait_until_request structure
/// \param req Pointer to request to initialize
/// \param uri Document URI
/// \param reason Save reason
static inline void lsp_text_document_will_save_wait_until_request_init(lsp_text_document_will_save_wait_until_request* req, const lsp_uri* uri, int reason) noexcept {
    req->uri.chars = uri->chars;
    req->reason = reason;
}

// ======================================================================================================================
// TEXT EDIT LIST UTILITIES
// ======================================================================================================================

/// \brief Initialize an lsp_text_edit_list structure as NONE (null)
/// \param list Pointer to text edit list to initialize
static inline void lsp_text_edit_list_init_none(lsp_text_edit_list* list) noexcept {
    list->kind = LSP_OPT_NONE;
}

/// \brief Initialize an lsp_text_edit_list structure as SOME (with edits)
/// \param list Pointer to text edit list to initialize
/// \param head Linked list of text edits
/// \param count Number of text edits
static inline void lsp_text_edit_list_init_some(lsp_text_edit_list* list, struct lsp_list<lsp_text_edit>* head, int count) noexcept {
    list->kind = LSP_OPT_SOME;
    list->head = head;
    list->count = count;
}

/// \brief Check if lsp_text_edit_list has edits
/// \param list Pointer to text edit list to check
/// \return true if list has edits
static inline bool lsp_text_edit_list_is_some(const lsp_text_edit_list* list) noexcept {
    return list->kind == LSP_OPT_SOME;
}

static inline struct lsp_notebook_document_did_open_notification* lsp_init_notebook_document_did_open_notification(void *mem, AkU64 tag_id) noexcept {
    struct lsp_notebook_document_did_open_notification* n = (struct lsp_notebook_document_did_open_notification*)mem;
    n->hdr.timestamp_nanos = ak_query_timer_ns();
    n->hdr.refcount.store(1, std::memory_order_relaxed);
    return n;
}
static inline struct lsp_notebook_document_did_change_notification* lsp_init_notebook_document_did_change_notification(void *mem, AkU64 tag_id) noexcept {
    struct lsp_notebook_document_did_change_notification* n = (struct lsp_notebook_document_did_change_notification*)mem;
    n->hdr.timestamp_nanos = ak_query_timer_ns();
    n->hdr.refcount.store(1, std::memory_order_relaxed);
    return n;
}
static inline struct lsp_notebook_document_did_save_notification* lsp_init_notebook_document_did_save_notification(void *mem, AkU64 tag_id) noexcept {
    struct lsp_notebook_document_did_save_notification* n = (struct lsp_notebook_document_did_save_notification*)mem;
    n->hdr.timestamp_nanos = ak_query_timer_ns();
    n->hdr.refcount.store(1, std::memory_order_relaxed);
    return n;
}
static inline struct lsp_notebook_document_did_close_notification* lsp_init_notebook_document_did_close_notification(void *mem, AkU64 tag_id) noexcept {
    struct lsp_notebook_document_did_close_notification* n = (struct lsp_notebook_document_did_close_notification*)mem;
    n->hdr.timestamp_nanos = ak_query_timer_ns();
    n->hdr.refcount.store(1, std::memory_order_relaxed);
    return n;
}

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
