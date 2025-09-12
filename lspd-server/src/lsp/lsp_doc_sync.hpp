#pragma once

#include "lsp_basic.hpp" // IWYU pragma: keep

/// \file lsp_doc_sync.hpp
/// \brief Text document synchronization message declarations
///
/// This file contains LSP messages related to text document synchronization,
/// including opening, changing, saving, and closing text documents.

/// \defgroup lsp_text_document Text Document Synchronization
/// \brief Messages for text document state synchronization
///
/// These messages handle opening, changing, saving, and closing text documents,
/// ensuring the server has the most current document state.

/// \ingroup lsp_text_document
/// \brief Identifier for a text document.
/// \details Carries the document URI and a version number. The version
/// increments on each change notification and is used for concurrency control.
struct lsp_text_document_id {
    /// \brief Document URI.
    struct lsp_uri uri;
    /// \brief Document version.
    int            version;
};

/// \ingroup lsp_text_document
/// \brief Internal helper notification carrying a document update slice.
/// \details Not an LSP wire message; used internally to model applying
/// a text change to a document range.
struct lsp_text_document_update_notification {
    /// \brief Common message header (timestamp, tags, etc.).
    struct lsp_msg_hdr          hdr;
    /// \brief Target document identifier (URI + version).
    struct lsp_text_document_id id;
    /// \brief Target range to replace.
    struct lsp_range            range;
    /// \brief Replacement text.
    struct lsp_string           text;
};

// textDocument/didOpen (notification)
/// \brief textDocument/didOpen notification.
/// \details Sent from the client when a text document is opened.
struct lsp_text_document_did_open_notification {
    /// \brief Common message header.
    struct lsp_msg_hdr            hdr;
    /// \brief Opened text document info.
    struct lsp_text_document_item text_document;
};
static struct lsp_text_document_did_open_notification* lsp_init_text_document_did_open_notification(void *mem, AkU64 tag_id) noexcept;

// textDocument/didChange (notification)
/// \brief Content change descriptor used by didChange.
struct lsp_text_document_content_change {
    /// \brief Range to replace.
    struct lsp_range  range;
    /// \brief Replacement text.
    struct lsp_string text;
};
/// \brief textDocument/didChange notification.
/// \details Sent from the client when document content changes.
struct lsp_text_document_did_change_notification {
    /// \brief Common message header.
    struct lsp_msg_hdr                                        hdr;
    /// \brief Versioned document identifier.
    struct lsp_versioned_text_document_id                     text_document;
    /// \brief Linked list of content changes.
    struct lsp_list<struct lsp_text_document_content_change>* changes;
};
static struct lsp_text_document_did_change_notification* lsp_init_text_document_did_change_notification(void *mem, AkU64 tag_id) noexcept;

// ------------------------------------------------------------------------------------------------------------------------------------------------------------------
// textDocument/didClose (notification)
// ------------------------------------------------------------------------------------------------------------------------------------------------------------------

// textDocument/didClose (notification)
/// \brief textDocument/didClose notification.
/// \details Sent from the client when a text document is closed.
struct lsp_text_document_did_close_notification {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Closed document URI.
    struct lsp_uri     uri;
};
static struct lsp_text_document_did_close_notification* lsp_init_text_document_did_close_notification(void *mem, AkU64 tag_id) noexcept;

// ------------------------------------------------------------------------------------------------------------------------------------------------------------------
// textDocument/didSave (notification)
// ------------------------------------------------------------------------------------------------------------------------------------------------------------------

/// \brief textDocument/didSave notification.
/// \details Sent when the document is saved. May include saved text.
struct lsp_text_document_did_save_notification {
    /// \brief Common message header.
    struct lsp_msg_hdr    hdr;
    /// \brief Saved document URI.
    struct lsp_uri        uri;
    /// \brief Optional saved text content.
    struct lsp_opt_string text;
};
static struct lsp_text_document_did_save_notification* lsp_init_text_document_did_save_notification(void *mem, AkU64 tag_id) noexcept;

// ------------------------------------------------------------------------------------------------------------------------------------------------------------------
// textDocument/willSave (notification)
// ------------------------------------------------------------------------------------------------------------------------------------------------------------------

/// \brief textDocument/willSave notification.
/// \details Sent before the document is saved; includes save reason.
struct lsp_text_document_will_save_notification {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Document URI.
    struct lsp_uri     uri;
    /// \brief Save reason (LSP enum 1..3).
    int                reason;
};
static struct lsp_text_document_will_save_notification* lsp_init_text_document_will_save_notification(void *mem, AkU64 tag_id) noexcept;

// ------------------------------------------------------------------------------------------------------------------------------------------------------------------
// textDocument/willSaveWaitUntil (request, response, partial_respone, error)
// ------------------------------------------------------------------------------------------------------------------------------------------------------------------

/// \brief textDocument/willSaveWaitUntil request.
/// \details Sent before saving; server may return TextEdit[] to apply.
struct lsp_text_document_will_save_wait_until_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Document URI.
    struct lsp_uri     uri;
    /// \brief Save reason (LSP enum 1..3).
    int                reason;
};
/// \brief Partial response for willSaveWaitUntil.
struct lsp_text_document_will_save_wait_until_partial_response {
    /// \brief Common message header.
    struct lsp_msg_hdr         hdr;
    /// \brief Partial TextEdit[] or null.
    struct lsp_text_edit_list  partial;
};
/// \brief Final response for willSaveWaitUntil.
struct lsp_text_document_will_save_wait_until_response {
    /// \brief Common message header.
    struct lsp_msg_hdr         hdr;
    /// \brief TextEdit[] or null.
    struct lsp_text_edit_list  result;
};
/// \brief Error result for willSaveWaitUntil.
struct lsp_text_document_will_save_wait_until_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr      hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_text_document_will_save_wait_until_request*          lsp_init_text_document_will_save_wait_until_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_will_save_wait_until_partial_response* lsp_init_text_document_will_save_wait_until_partial_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_will_save_wait_until_response*         lsp_init_text_document_will_save_wait_until_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_will_save_wait_until_error_result*     lsp_init_text_document_will_save_wait_until_error_result(void *mem, AkU64 tag_id) noexcept;


