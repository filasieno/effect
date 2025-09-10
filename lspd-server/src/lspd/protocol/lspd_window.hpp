#pragma once

#include "lspd_basic.hpp" // IWYU pragma: keep

/// \file lspd_window.hpp
/// \brief Window and UI message declarations (showMessage, logMessage, showDocument)
///
/// This file contains LSP messages related to window and user interface interactions,
/// including showing messages, displaying documents, and handling user responses.

/// \defgroup lsp_window_types Window and UI Types
/// \brief Types for window and user interface interactions
///
/// Types used specifically for window/UI operations like showing messages
/// and displaying documents.

/// \ingroup lsp_window_types
/// \brief Message type enumeration
///
/// Defines the severity level of messages displayed to the user.
/// Used to categorize the importance and visual styling of messages.
enum lsp_message_type {
    LSP_MSG_ERROR = 1,    ///< Error message - highest severity
    LSP_MSG_WARNING = 2,  ///< Warning message - medium severity
    LSP_MSG_INFO = 3,     ///< Informational message - normal severity
    LSP_MSG_LOG = 4       ///< Log message - lowest severity
};

/// Window log message notification parameters
struct lsp_log_message_params {
    enum lsp_message_type type;
    struct lsp_string message;
};

/// \ingroup lsp_window_types
/// \brief Message action item
///
/// Represents an action that can be taken in response to a message.
/// Used in showMessageRequest for user interaction and choice presentation.
struct lsp_message_action_item {
    lsp_string title;  ///< Action title displayed to user
};

/// \ingroup lsp_window_types
/// \brief List of message action items
///
/// Contains a list of actions that can be presented to the user
/// in response to a message request.
struct lsp_action_item_list {
    struct lsp_list<struct lsp_message_action_item>* head;  ///< Head of action list
    int count;                                              ///< Number of actions
};

/// \ingroup lsp_window_types
/// \brief Optional message action item
///
/// Represents either a message action item or null.
/// Used for responses that may or may not contain a selected action.
struct lsp_opt_action_item {
    int kind;  ///< LSP_OPT_NONE (null) or LSP_OPT_SOME (action present)
    struct lsp_message_action_item value;  ///< Action item (valid if kind == SOME)
};

/// \ingroup lsp_window_types
/// \brief Show document parameters
///
/// Parameters for requesting the client to show a document,
/// optionally with selection and focus behavior.
struct lsp_show_document_params {
    lsp_uri uri;                           ///< Document URI to show
    lsp_opt_bool external;                 ///< Show in external program
    lsp_opt_bool take_focus;               ///< Take focus when showing
    lsp_opt_string selection_present;      ///< Whether selection is specified
    lsp_range selection;                   ///< Selection range (if present)
};

/// \ingroup lsp_window_types
/// \brief Show document result
///
/// Result indicating whether the document was successfully shown.
/// Provides feedback about the success of the show document operation.
struct lsp_show_document_result {
    bool success;  ///< True if document was shown successfully
};

/// \ingroup lsp_window_types
/// \brief Show message parameters
///
/// Parameters for displaying a message to the user.
/// Contains the message type/severity and the message text.
struct lsp_show_message_params {
    enum lsp_message_type type;  ///< Message type/severity
    lsp_string message;          ///< Message text
};

/// \ingroup lsp_window_types
/// \brief Show message request parameters
///
/// Parameters for displaying a message to the user with possible actions.
/// Allows the server to present choices to the user and receive a response.
struct lsp_show_message_request_params {
    enum lsp_message_type type;             ///< Message type/severity
    lsp_string message;                     ///< Message text
    struct lsp_action_item_list actions;    ///< Available actions
};

/// The `window/showDocument` request is sent from the server to the client to show a document.
/// The client might open an external program depending on the URI.
/// Params: ShowDocumentParams. Result: ShowDocumentResult.
/// See LSP 3.17: Request window/showDocument
/// \brief window/showDocument request message.
/// \details Request sent from server to client to show a document.
/// \since 3.16.0
struct lsp_window_show_document_request {
    /// \brief Header.
    struct lsp_msg_hdr hdr;
    /// \brief Request params.
    struct lsp_show_document_params params;
};
/// \brief Partial response for window/showDocument.
struct lsp_window_show_document_partial_response {
    /// \brief Partial response header.
    struct lsp_msg_hdr hdr;
};
/// \brief Final response for window/showDocument.
struct lsp_window_show_document_response {
    /// \brief Header.
    struct lsp_msg_hdr hdr;
    /// \brief Result.
    struct lsp_show_document_result result;
};
/// \brief Error result for window/showDocument.
struct lsp_window_show_document_error_result {
    /// \brief Header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_window_show_document_request*          lsp_init_window_show_document_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_window_show_document_partial_response* lsp_init_window_show_document_partial_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_window_show_document_response*         lsp_init_window_show_document_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_window_show_document_error_result*     lsp_init_window_show_document_error_result(void *mem, AkU64 tag_id) noexcept;

/// The `window/showMessage` notification is sent from the server to the client to display a message.
/// Params: ShowMessageParams.
/// See LSP 3.17: Notification window/showMessage
struct lsp_window_show_message_notification {
    /// \brief Header.
    struct lsp_msg_hdr hdr;
    /// \brief Params.
    struct lsp_show_message_params params;
};
static struct lsp_window_show_message_notification* lsp_init_window_show_message_notification(void *mem, AkU64 tag_id) noexcept;

/// The `window/showMessageRequest` request is sent from the server to the client to display a message and await a user action.
/// Params: ShowMessageRequestParams. Result: MessageActionItem | null.
/// See LSP 3.17: Request window/showMessageRequest
struct lsp_window_show_message_request {
    /// \brief Header.
    struct lsp_msg_hdr hdr;
    /// \brief Params.
    struct lsp_show_message_request_params params;
};
struct lsp_window_show_message_partial_response {
    /// \brief Partial response header.
    struct lsp_msg_hdr hdr;
};
struct lsp_window_show_message_response {
    /// \brief Header.
    struct lsp_msg_hdr hdr;
    /// \brief Optional selected action.
    struct lsp_opt_action_item result;
};
/// \brief Error result for window/showMessageRequest.
struct lsp_window_show_message_error_result {
    /// \brief Header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_window_show_message_request*          lsp_init_window_show_message_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_window_show_message_partial_response* lsp_init_window_show_message_partial_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_window_show_message_response*         lsp_init_window_show_message_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_window_show_message_error_result*     lsp_init_window_show_message_error_result(void *mem, AkU64 tag_id) noexcept;


