#pragma once

#include "lsp_basic.hpp" // IWYU pragma: keep
#include "lsp_dyn.hpp"


/// \defgroup lsp_workspace_types Workspace Types
/// \brief Types for workspace-level operations and features
///
/// Types used specifically for workspace operations like symbol search,
/// configuration management, file operations, and workspace edits.

/// \ingroup lsp_workspace_types
/// \brief Workspace folder
///
/// Represents a workspace folder that the language server
/// should be aware of and operate within.
struct lsp_workspace_folder {
    lsp_uri uri;        ///< Folder URI
    lsp_string name;    ///< Display name
};

/// \ingroup lsp_workspace_types
/// \brief Workspace symbol parameters
///
/// Parameters for workspace-wide symbol search requests.
/// Contains the query string to match against symbol names.
struct lsp_workspace_symbol_params {
    lsp_string query;  ///< Query string to search for symbols
};

/// \ingroup lsp_workspace_types
/// \brief Symbol information list
///
/// Contains a list of symbol information entries for workspace symbol results.
struct lsp_symbol_information_list {
    struct lsp_list<struct lsp_symbol_information>* head;  ///< List of symbol information
    int count;                                             ///< Number of symbols
};

/// \ingroup lsp_workspace_types
/// \brief Execute command parameters
///
/// Parameters for executing a command in the workspace.
/// Contains the command identifier and optional arguments.
struct lsp_execute_command_params {
    lsp_string command;                              ///< Command identifier
    struct lsp_list<struct lsp_dyn>* arguments;      ///< Command arguments (LSPAny array)
};



/// \ingroup lsp_workspace_types
/// \brief Apply workspace edit parameters
///
/// Parameters for applying a workspace edit.
/// Contains an optional label and the workspace edit to apply.
struct lsp_apply_workspace_edit_params {
    lsp_string label;                    ///< Optional label for the edit
    struct lsp_workspace_edit edit;      ///< The workspace edit to apply
};

/// \ingroup lsp_workspace_types
/// \brief Apply workspace edit result
///
/// Result of applying a workspace edit.
/// Indicates whether the edit was successfully applied.
struct lsp_apply_workspace_edit_result {
    bool applied;                      ///< Whether the edit was applied
    lsp_opt_uinteger failure_reason;   ///< Optional failure reason code
};

/// \ingroup lsp_workspace_types
/// \brief File change type enumeration
///
/// Defines the types of file changes that can occur in the workspace.
/// Used to categorize file system events.
enum lsp_file_change_type {
    LSP_FILE_CREATED = 1,  ///< File was created
    LSP_FILE_CHANGED = 2,  ///< File was modified
    LSP_FILE_DELETED = 3   ///< File was deleted
};

/// \ingroup lsp_workspace_types
/// \brief File event
///
/// Represents a file system event with the affected file URI and change type.
struct lsp_file_event {
    lsp_uri uri;                           ///< Affected file URI
    enum lsp_file_change_type type;        ///< Type of change
};

/// \ingroup lsp_workspace_types
/// \brief File event list
///
/// Contains a list of file events for workspace change notifications.
struct lsp_file_event_list {
    struct lsp_list<struct lsp_file_event>* head;  ///< List of file events
    int count;                                     ///< Number of events
};

/// \ingroup lsp_workspace_types
/// \brief Did change workspace folders parameters
///
/// Parameters for workspace folders change notifications.
/// Contains lists of added and removed workspace folders.
struct lsp_did_change_workspace_folders_params {
    struct lsp_list<struct lsp_workspace_folder>* added;    ///< Added workspace folders
    struct lsp_list<struct lsp_workspace_folder>* removed;  ///< Removed workspace folders
};

/// \ingroup lsp_workspace_types
/// \brief Workspace folders request parameters
///
/// Parameters for workspace folders requests.
/// Currently empty as per LSP specification.
struct lsp_workspace_foldders_request_params {
    // Empty as per LSP spec
};

/// \ingroup lsp_workspace_types
/// \brief Workspace folders result
///
/// Result of workspace folders requests.
/// Contains an optional list of workspace folders.
struct lsp_workspace_folders_result {
    int kind;  ///< LSP_OPT_NONE or LSP_OPT_SOME
    struct lsp_list<struct lsp_workspace_folder>* value;  ///< Workspace folders (if present)
};

/// \ingroup lsp_workspace_types
/// \brief Configuration item
///
/// Represents a single configuration item request.
/// Specifies the scope URI and configuration section to retrieve.
struct lsp_configuration_item {
    lsp_opt_string scope_uri;  ///< Optional scope URI
    lsp_opt_string section;    ///< Configuration section
};

/// \ingroup lsp_workspace_types
/// \brief Configuration parameters
///
/// Parameters for configuration requests.
/// Contains a list of configuration items to retrieve.
struct lsp_configuration_params {
    struct lsp_list<struct lsp_configuration_item>* items;  ///< Configuration items to retrieve
};

/// \ingroup lsp_workspace_types
/// \brief Configuration result
///
/// Result of configuration requests.
/// Contains the requested configuration values as LSPAny objects.
struct lsp_configuration_result {
    struct lsp_list<struct lsp_dyn>* items;  ///< Configuration values (LSPAny array)
};


/// The `workspace/symbol` request is sent from the client to the server to list project-wide symbols matching the query string.
/// The request's parameter is of type {@link WorkspaceSymbolParams} and the response is of type {@link SymbolInformation SymbolInformation[]} or a Thenable that resolves to such.
/// \since 3.0.0
struct lsp_workspace_symbol_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Workspace symbol parameters.
    struct lsp_workspace_symbol_params params;
};
/// \brief Final workspace symbols response.
struct lsp_workspace_symbol_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Array of symbol information or null.
    struct lsp_symbol_information_list result;
};
/// \brief Error result for workspace symbol request.
struct lsp_workspace_symbol_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_workspace_symbol_request*      lsp_init_workspace_symbol_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_workspace_symbol_response*     lsp_init_workspace_symbol_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_workspace_symbol_error_result* lsp_init_workspace_symbol_error_result(void *mem, AkU64 tag_id) noexcept;

/// The `workspaceSymbol/resolve` request is sent from the client to the server to resolve additional information for a given workspace symbol.
/// The request's parameter is of type {@link SymbolInformation} and the response is of type {@link SymbolInformation} or a Thenable that resolves to such.
/// \since 3.17.0
struct lsp_workspace_symbol_resolve_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Symbol information to resolve.
    struct lsp_symbol_information params;
};
/// \brief Final resolved workspace symbol.
struct lsp_workspace_symbol_resolve_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Resolved symbol information with additional details.
    struct lsp_symbol_information result;
};
/// \brief Error result for workspace symbol resolve.
struct lsp_workspace_symbol_resolve_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_workspace_symbol_resolve_request*      lsp_init_workspace_symbol_resolve_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_workspace_symbol_resolve_response*     lsp_init_workspace_symbol_resolve_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_workspace_symbol_resolve_error_result* lsp_init_workspace_symbol_resolve_error_result(void *mem, AkU64 tag_id) noexcept;

/// The `workspace/executeCommand` request is sent from the client to the server to trigger command execution on the server.
/// The request's parameter is of type {@link ExecuteCommandParams} and the response is of type {@link LSPAny} or a Thenable that resolves to such.
/// \since 3.0.0
struct lsp_workspace_execute_command_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Execute command parameters.
    struct lsp_execute_command_params params;
};
/// \brief Final execute command response.
struct lsp_workspace_execute_command_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Command execution result or null.
    struct lsp_dyn result;
};
/// \brief Error result for execute command request.
struct lsp_workspace_execute_command_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_workspace_execute_command_request*      lsp_init_workspace_execute_command_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_workspace_execute_command_response*     lsp_init_workspace_execute_command_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_workspace_execute_command_error_result* lsp_init_workspace_execute_command_error_result(void *mem, AkU64 tag_id) noexcept;

/// The `workspace/applyEdit` request is sent from the server to the client to modify resource on the client side.
/// The request's parameter is of type {@link ApplyWorkspaceEditParams} and the response is of type {@link ApplyWorkspaceEditResult} or a Thenable that resolves to such.
/// \since 3.0.0
struct lsp_workspace_apply_edit_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Apply workspace edit parameters.
    struct lsp_apply_workspace_edit_params params;
};
/// \brief Final apply edit response.
struct lsp_workspace_apply_edit_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Apply workspace edit result.
    struct lsp_apply_workspace_edit_result result;
};
/// \brief Error result for apply edit request.
struct lsp_workspace_apply_edit_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_workspace_apply_edit_request*      lsp_init_workspace_apply_edit_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_workspace_apply_edit_response*     lsp_init_workspace_apply_edit_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_workspace_apply_edit_error_result* lsp_init_workspace_apply_edit_error_result(void *mem, AkU64 tag_id) noexcept;

/// The `workspace/didChangeWorkspaceFolders` notification is sent from the client to the server to inform about workspace folder configuration changes.
/// Params: DidChangeWorkspaceFoldersParams.
/// \since 3.6.0
struct lsp_workspace_did_change_workspace_folders_notification {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Workspace folder change parameters.
    struct lsp_did_change_workspace_folders_params params;
};
static struct lsp_workspace_did_change_workspace_folders_notification* lsp_init_workspace_did_change_workspace_folders_notification(void *mem, AkU64 tag_id) noexcept;

/// The `workspace/configuration` request is sent from the server to the client to fetch configuration settings from the client.
/// The request's parameter is of type {@link ConfigurationParams} and the response is of type {@link LSPAny LSPAny[]} or a Thenable that resolves to such.
/// \since 3.6.0
struct lsp_workspace_configuration_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Configuration parameters.
    struct lsp_configuration_params params;
};
/// \brief Final configuration response.
struct lsp_workspace_configuration_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Array of configuration values.
    struct lsp_configuration_result result;
};
/// \brief Error result for configuration request.
struct lsp_workspace_configuration_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_workspace_configuration_request*      lsp_init_workspace_configuration_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_workspace_configuration_response*     lsp_init_workspace_configuration_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_workspace_configuration_error_result* lsp_init_workspace_configuration_error_result(void *mem, AkU64 tag_id) noexcept;

/// The `workspace/workspaceFolders` request is sent from the server to the client to fetch the open workspace folders.
/// Result: WorkspaceFolder[] | null.
/// See LSP 3.17: Request workspace/workspaceFolders
/// \brief workspace/workspaceFolders request message.
struct lsp_workspace_folders_request {
    /// \brief Request header.
    struct lsp_msg_hdr hdr;
};

/// \brief Partial response for workspace/workspaceFolders.
struct lsp_workspace_folders_partial_response 
{
    /// \brief Partial response header.
    struct lsp_msg_hdr hdr;
};
struct lsp_workspace_folders_response 
{
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Presence of the result array (null or some).
    int                kind;  /// LSP_OPT_NONE => null, LSP_OPT_SOME => array provided
    /// \brief Head of workspace folder list when present.
    struct lsp_list<struct lsp_workspace_folder>* head;
    /// \brief Number of folders when present.
    int                count;
};

/// \brief Error result for workspace/workspaceFolders.
struct lsp_workspace_folders_error_result 
{
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_workspace_folders_request*          lsp_init_workspace_folders_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_workspace_folders_partial_response* lsp_init_workspace_folders_partial_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_workspace_folders_response*         lsp_init_workspace_folders_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_workspace_folders_error_result*     lsp_init_workspace_folders_error_result(void *mem, AkU64 tag_id) noexcept;




/// Did change watched files parameters
struct lsp_did_change_watched_files_params
{
    struct lsp_file_event_list changes;
};


