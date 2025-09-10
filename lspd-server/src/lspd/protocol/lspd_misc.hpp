#pragma once

#include "lspd_basic.hpp" // IWYU pragma: keep

/// \file lspd_misc.hpp
/// \brief Miscellaneous message declarations (progress, telemetry, cancel, registration)

// ======================================================================================================================
// WORK DONE PROGRESS TYPES
// ======================================================================================================================
enum lsp_work_done_progress_kind {
    LSP_WORK_DONE_BEGIN = 0,
    LSP_WORK_DONE_REPORT,
    LSP_WORK_DONE_END,
};
/// \ingroup lsp_misc
/// \brief Work done progress begin notification parameters
///
/// Parameters for the work done progress begin notification.
/// \since 3.15.0
struct lsp_work_done_progress_begin {
    enum lsp_work_done_progress_kind kind;  ///< Always LSP_WORK_DONE_BEGIN
    lsp_string       title;                  ///< Mandatory title of the progress
    lsp_opt_bool     cancellable;            ///< Whether the operation is cancellable
    lsp_opt_string   message;                ///< Optional message
    lsp_opt_uinteger percentage;             ///< Optional percentage (0-100)
};

/// \ingroup lsp_misc
/// \brief Work done progress report notification parameters
///
/// Parameters for the work done progress report notification.
/// \since 3.15.0
struct lsp_work_done_progress_report {
    enum lsp_work_done_progress_kind kind;  ///< Always LSP_WORK_DONE_REPORT
    lsp_opt_bool     cancellable;            ///< Whether the operation is cancellable
    lsp_opt_string   message;                ///< Optional message
    lsp_opt_uinteger percentage;             ///< Optional percentage (0-100)
};

/// \ingroup lsp_misc
/// \brief Work done progress end notification parameters
///
/// Parameters for the work done progress end notification.
/// \since 3.15.0
struct lsp_work_done_progress_end {
    enum lsp_work_done_progress_kind kind;  ///< Always LSP_WORK_DONE_END
    lsp_opt_string message;                  ///< Optional message
};

/// \ingroup lsp_misc
/// \brief Work done progress create request parameters
///
/// Parameters for the window/workDoneProgress/create request.
/// \since 3.15.0
struct lsp_work_done_progress_create_params {
    lsp_dyn token;  ///< Progress token (string | number)
};

/// \ingroup lsp_misc
/// \brief Work done progress cancel request parameters
///
/// Parameters for the window/workDoneProgress/cancel request.
/// \since 3.15.0
struct lsp_work_done_progress_cancel_params {
    lsp_dyn token;  ///< Progress token (string | number)
};

// ======================================================================================================================
// TRACE VALUE TYPES
// ======================================================================================================================

/// \ingroup lsp_misc
/// \brief Trace value enumeration
///
/// Defines the verbosity level of the trace messages.
/// \since 3.16.0
enum lsp_trace_value {
    LSP_TRACE_OFF = 0,      ///< No tracing
    LSP_TRACE_MESSAGES = 1,  ///< Trace messages only
    LSP_TRACE_VERBOSE = 2    ///< Verbose tracing
};

/// \ingroup lsp_misc
/// \brief Set trace notification parameters
///
/// Parameters for the $/setTrace notification.
/// \since 3.16.0
struct lsp_set_trace_notification_params {
    enum lsp_trace_value value;  ///< New trace value
};

/// \ingroup lsp_misc
/// \brief Log trace notification parameters
///
/// Parameters for the $/logTrace notification.
/// \since 3.16.0
struct lsp_log_trace_notification_params {
    lsp_string     message;  ///< Trace message
    lsp_opt_string verbose;  ///< Optional verbose information
};

// ======================================================================================================================
// CANCEL REQUEST TYPES
// ======================================================================================================================

/// \ingroup lsp_misc
/// \brief Cancel request parameters
///
/// Parameters for the $/cancelRequest notification.
/// Used to cancel an ongoing request.
struct lsp_cancel_request_params {
    lsp_dyn id;  ///< Request ID to cancel (number | string)
};

// ======================================================================================================================
// TELEMETRY TYPES
// ======================================================================================================================


// ======================================================================================================================
// FILE RESOURCE CHANGES TYPES
// ======================================================================================================================

/// \ingroup lsp_misc
/// \brief File change type enumeration
///
/// Defines the type of change that occurred to a file.
/// Used in file system watching and workspace operations.
/// \since 3.13.0
enum lsp_file_change_type {
    LSP_FILE_CREATED = 1,  ///< File was created
    LSP_FILE_CHANGED = 2,  ///< File was modified
    LSP_FILE_DELETED = 3   ///< File was deleted
};

/// \ingroup lsp_misc
/// \brief File event structure
///
/// Represents a file system event.
/// Used in didChangeWatchedFiles notifications.
/// \since 3.13.0
struct lsp_file_event {
    lsp_uri uri;                           ///< Affected file URI
    enum lsp_file_change_type type;        ///< Type of change
};

/// \ingroup lsp_misc
/// \brief File event list
///
/// Contains a list of file events.
/// Used in didChangeWatchedFiles notifications.
/// \since 3.13.0
struct lsp_file_event_list {
    struct lsp_list<struct lsp_file_event>* head;  ///< List of file events
    int count;                                     ///< Number of events
};

// ======================================================================================================================
// INITIALIZATION AND CONFIGURATION TYPES
// ======================================================================================================================


/// \ingroup lsp_misc
/// \brief Initialized notification parameters
///
/// Parameters for the initialized notification.
/// Empty by design - just signals that initialization is complete.
/// \since 3.0.0
struct lsp_initialized_params {
    // Empty - no parameters needed
};

/// \ingroup lsp_misc
/// \brief Shutdown request parameters
///
/// Parameters for the shutdown request.
/// Empty by design - no parameters needed.
struct lsp_shutdown_params {
    // Empty - no parameters needed
};

/// \ingroup lsp_misc
/// \brief Shutdown result
///
/// Result returned from the shutdown request.
/// Null by design - just acknowledges shutdown.
struct lsp_shutdown_result {
    // Null result
};

/// \ingroup lsp_misc
/// \brief Exit notification parameters
///
/// Parameters for the exit notification.
/// Empty by design - no parameters needed.
struct lsp_exit_params {
    // Empty - no parameters needed
};

// ======================================================================================================================
// CAPABILITY TYPES
// ======================================================================================================================

/// \ingroup lsp_misc
/// \brief Server capabilities structure
///
/// Defines the capabilities that the language server supports.
/// Sent during initialization to inform the client of available features.
/// \since 3.0.0
struct lsp_server_capabilities {
    // Core capabilities
    lsp_opt_string text_document_sync;         ///< Text document synchronization capability

    // Language features - using optional strings to represent various capability types
    lsp_opt_string hover_provider;             ///< Hover capability
    lsp_opt_string completion_provider;        ///< Completion capability
    lsp_opt_string signature_help_provider;    ///< Signature help capability
    lsp_opt_string definition_provider;        ///< Definition capability
    lsp_opt_string type_definition_provider;   ///< Type definition capability
    lsp_opt_string implementation_provider;    ///< Implementation capability
    lsp_opt_string references_provider;        ///< References capability
    lsp_opt_string document_highlight_provider; ///< Document highlight capability
    lsp_opt_string document_symbol_provider;   ///< Document symbol capability
    lsp_opt_string workspace_symbol_provider;  ///< Workspace symbol capability
    lsp_opt_string code_action_provider;       ///< Code action capability
    lsp_opt_string code_lens_provider;         ///< Code lens capability
    lsp_opt_string document_formatting_provider; ///< Document formatting capability
    lsp_opt_string document_range_formatting_provider; ///< Document range formatting capability
    lsp_opt_string document_on_type_formatting_provider; ///< On-type formatting capability
    lsp_opt_string rename_provider;            ///< Rename capability
    lsp_opt_string document_link_provider;     ///< Document link capability
    lsp_opt_string color_provider;             ///< Color capability
    lsp_opt_string folding_range_provider;     ///< Folding range capability
    lsp_opt_string declaration_provider;       ///< Declaration capability
    lsp_opt_string execute_command_provider;   ///< Execute command capability
    lsp_opt_string workspace_folders_provider; ///< Workspace folders capability
    lsp_opt_string semantic_tokens_provider;   ///< Semantic tokens capability
    lsp_opt_string moniker_provider;           ///< Moniker capability
    lsp_opt_string linked_editing_range_provider; ///< Linked editing range capability
    lsp_opt_string call_hierarchy_provider;    ///< Call hierarchy capability
    lsp_opt_string type_hierarchy_provider;    ///< Type hierarchy capability
    lsp_opt_string inline_value_provider;      ///< Inline value capability
    lsp_opt_string inlay_hint_provider;        ///< Inlay hint capability
    lsp_opt_string diagnostic_provider;        ///< Diagnostic capability

    // Experimental features
    lsp_opt_string experimental;               ///< Experimental capabilities
};

/// \ingroup lsp_misc
/// \brief Client capabilities structure
///
/// Defines the capabilities that the language client supports.
/// Sent during initialization to inform the server of available features.
/// \since 3.0.0
struct lsp_client_capabilities {
    // Workspace capabilities
    lsp_opt_string workspace;              ///< Workspace capabilities

    // Text document capabilities
    lsp_opt_string text_document;          ///< Text document capabilities

    // Window capabilities
    lsp_opt_string window;                 ///< Window capabilities

    // General capabilities
    lsp_opt_string general;                ///< General capabilities

    // Experimental features
    lsp_opt_string experimental;           ///< Experimental capabilities
};

// ======================================================================================================================
// MESSAGE TYPES
// ======================================================================================================================

/// \ingroup lsp_misc
/// \brief Message types for window/showMessage
///
/// Defines the severity levels for messages displayed to the user.
/// \since 3.0.0
enum lsp_message_type {
    LSP_MESSAGE_ERROR = 1,    ///< Error message - highest severity
    LSP_MESSAGE_WARNING = 2,  ///< Warning message - medium severity
    LSP_MESSAGE_INFO = 3,     ///< Informational message - normal severity
    LSP_MESSAGE_LOG = 4       ///< Log message - lowest severity
};

/// \ingroup lsp_misc
/// \brief Show message parameters
///
/// Parameters for window/showMessage and window/showMessageRequest.
/// \since 3.0.0
struct lsp_show_message_params {
    enum lsp_message_type type;  ///< Message type/severity
    lsp_string message;          ///< Message text
};

/// \ingroup lsp_misc
/// \brief Show message request parameters
///
/// Parameters for window/showMessageRequest.
/// Includes action items that the user can select.
/// \since 3.0.0
struct lsp_show_message_request_params {
    enum lsp_message_type type;             ///< Message type/severity
    lsp_string message;                     ///< Message text
    struct lsp_list<struct lsp_message_action_item>* actions;  ///< Available actions
};

/// \ingroup lsp_misc
/// \brief Message action item
///
/// Represents an action that can be taken in response to a message.
/// \since 3.0.0
struct lsp_message_action_item {
    lsp_string title;  ///< Action title displayed to user
};

/// \ingroup lsp_misc
/// \brief Message action item list
///
/// Contains a list of message action items.
/// \since 3.0.0
struct lsp_message_action_item_list {
    struct lsp_list<struct lsp_message_action_item>* head;  ///< Head of action list
    int count;                                              ///< Number of actions
};

/// \ingroup lsp_misc
/// \brief Optional message action item
///
/// Represents an optional message action item.
/// \since 3.0.0
struct lsp_opt_message_action_item {
    int kind;  ///< LSP_OPT_NONE (null) or LSP_OPT_SOME (action present)
    struct lsp_message_action_item value;  ///< Action item (valid if kind == SOME)
};

/// \ingroup lsp_misc
/// \brief Show message request result
///
/// Result returned from window/showMessageRequest.
/// \since 3.0.0
struct lsp_show_message_request_result {
    struct lsp_opt_message_action_item result;  ///< Selected action (null if none)
};

/// \ingroup lsp_misc
/// \brief Show document parameters
///
/// Parameters for window/showDocument.
/// \since 3.17.0
struct lsp_show_document_params {
    lsp_uri uri;                           ///< Document URI to show
    lsp_opt_bool external;                 ///< Show in external program
    lsp_opt_bool take_focus;               ///< Take focus when showing
    lsp_opt_string selection_present;      ///< Whether selection is specified
    lsp_range selection;                   ///< Selection range (if present)
};

/// \ingroup lsp_misc
/// \brief Show document result
///
/// Result returned from window/showDocument.
/// \since 3.17.0
struct lsp_show_document_result {
    bool success;  ///< True if document was shown successfully
};

/// \ingroup lsp_misc
/// \brief Log message parameters
///
/// Parameters for window/logMessage.
/// \since 3.0.0
struct lsp_log_message_params {
    enum lsp_message_type type;  ///< Message type/severity
    lsp_string message;          ///< Message text
};

// ======================================================================================================================
// CAPABILITY REGISTRATION TYPES
// ======================================================================================================================


// ======================================================================================================================
// CONFIGURATION TYPES
// ======================================================================================================================

/// \ingroup lsp_misc
/// \brief Configuration item structure
///
/// Represents a configuration item for workspace/configuration.
/// \since 3.6.0
struct lsp_configuration_item {
    lsp_opt_string scope_uri;  ///< Optional scope URI
    lsp_opt_string section;    ///< Configuration section
};

/// \ingroup lsp_misc
/// \brief Configuration parameters
///
/// Parameters for the workspace/configuration request.
/// \since 3.6.0
struct lsp_configuration_params {
    struct lsp_list<struct lsp_configuration_item>* items;  ///< Configuration items
};

/// \ingroup lsp_misc
/// \brief Configuration result
///
/// Result returned from workspace/configuration.
/// \since 3.6.0
struct lsp_configuration_result {
    struct lsp_list<struct lsp_dyn>* items;  ///< Configuration values (LSPAny array)
};


/// \ingroup lsp_misc
/// \brief Did change watched files parameters
///
/// Parameters for the workspace/didChangeWatchedFiles notification.
/// \since 3.6.0
struct lsp_did_change_watched_files_params {
    struct lsp_file_event_list changes;  ///< File changes
};

// ======================================================================================================================
// EXECUTE COMMAND TYPES
// ======================================================================================================================

/// \ingroup lsp_misc
/// \brief Execute command parameters
///
/// Parameters for the workspace/executeCommand request.
/// \since 3.6.0
struct lsp_execute_command_params {
    lsp_string command;                              ///< Command identifier
    struct lsp_list<struct lsp_dyn>* arguments;      ///< Command arguments (LSPAny array)
};

/// \ingroup lsp_misc
/// \brief Execute command result
///
/// Result returned from workspace/executeCommand.
/// \since 3.6.0
struct lsp_execute_command_result {
    struct lsp_dyn result;  ///< Command result (LSPAny)
};

/// \brief Workspace edit
///
/// Represents a set of changes to be applied to the workspace.
/// Contains document changes and deprecated changes field.
struct lsp_workspace_edit {
    struct lsp_list<struct lsp_text_document_edit>* document_changes;  ///< Document-specific changes
    struct lsp_list<struct lsp_text_edit>* changes;                    ///< Deprecated changes (use document_changes)
};

