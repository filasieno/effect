#pragma once
#include "ak/base/base_api.hpp"

/// \file lspd_basic.hpp
/// \brief Base/common LSP message declarations shared across groups


// -----------------------------------------------------------------------------------------------------------------------------------------------------------------
// File documentation
// -----------------------------------------------------------------------------------------------------------------------------------------------------------------

/// \file lspd_msg.hpp
/// \brief LSP Protocol Message Definitions
///
/// This file defines all LSP 3.17 protocol messages for the language server implementation.
/// Each message follows the LSP specification and includes comprehensive Doxygen documentation.
///
/// \see https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/
/// \since 3.17.0


/// \defgroup lsp_messages LSP Protocol Messages
/// \brief All LSP protocol message structures and types
///
/// This module contains all message structures defined in the LSP 3.17 specification,
/// organized by functional area. Each message includes initialization functions,
/// comprehensive documentation, and follows the LSP specification exactly.
///
/// \see https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/

/// \defgroup lsp_lifecycle Lifecycle Messages
/// \ingroup lsp_messages
/// \brief Messages for LSP session lifecycle management
///
/// These messages handle the initialization, configuration, and termination
/// of LSP sessions between clients and servers.

/// \defgroup lsp_text_document Text Document Synchronization
/// \ingroup lsp_messages
/// \brief Messages for text document state synchronization
///
/// These messages handle opening, changing, saving, and closing text documents,
/// ensuring the server has the most current document state.

/// \defgroup lsp_language_features Language Features
/// \ingroup lsp_messages
/// \brief Messages for LSP language features
///
/// These messages implement the core language intelligence features like
/// completion, hover, signature help, references, symbols, and code actions.

/// \defgroup lsp_workspace Workspace Features
/// \ingroup lsp_messages
/// \brief Messages for workspace-level operations
///
/// These messages handle workspace-wide operations like symbol search,
/// configuration management, and file operations.

/// \defgroup lsp_window Window Features
/// \ingroup lsp_messages
/// \brief Messages for window and UI interactions
///
/// These messages handle user interface interactions like showing messages,
/// displaying documents, and progress reporting.

/// \defgroup lsp_advanced Advanced Features
/// \ingroup lsp_messages
/// \brief Advanced LSP features (LSP 3.16+)
///
/// These messages implement newer LSP features like semantic tokens,
/// call hierarchy, type hierarchy, and diagnostics.

// All structures have been moved to their respective feature-specific files:
// - lspd_lifecycle.hpp: initialize, shutdown, initialized, exit
// - lspd_doc_sync.hpp: text document synchronization
// - lspd_workspace.hpp: workspace features
// - lspd_window.hpp: window/UI features
// - lspd_language.hpp: language features

/// \ingroup lsp_base_types
/// \brief Message direction enumeration
///
/// Specifies the direction of LSP message flow between client and server.
/// Used for method routing and protocol validation.
enum lsp_direction {
    LSP_DIR_C2S = 0,  ///< Client to server message
    LSP_DIR_S2C = 1   ///< Server to client message
};

/// \ingroup lsp_base_types
/// \brief High-level message interaction kind
///
/// Categorizes LSP messages by their interaction pattern.
/// Used for timing, routing, diagnostics, and state management.
/// Values must remain stable across versions.
enum lsp_interaction_kind {
    LSP_INTERACT_INVALID = 0,        ///< Invalid or unknown interaction
    LSP_INTERACT_NOTIFICATION,       ///< One-way notification (no response expected)
    LSP_INTERACT_REQUEST,            ///< Request requiring a response
    LSP_INTERACT_PARTIAL_RESPONSE,   ///< Partial response in streaming scenarios
    LSP_INTERACT_RESPONSE,           ///< Final response to a request
    LSP_INTERACT_ERROR_RESULT        ///< Error response to a request
};

/// \ingroup lsp_document_symbol_types
/// \brief Symbol tag enumeration
///
/// Additional properties that can be attached to symbols to provide
/// extra information about their status or usage.
enum lsp_symbol_tag {
    LSP_SYMBOL_DEPRECATED = 1  ///< Symbol is deprecated
};

/// \ingroup lsp_document_symbol_types
/// \brief Symbol kind enumeration
///
/// Defines all possible kinds of symbols that can be found in source code.
/// Used to categorize symbols in document symbol responses.
enum lsp_symbol_kind {
    LSP_SYMBOL_FILE = 1,           ///< File symbol
    LSP_SYMBOL_MODULE = 2,         ///< Module symbol
    LSP_SYMBOL_NAMESPACE = 3,      ///< Namespace symbol
    LSP_SYMBOL_PACKAGE = 4,        ///< Package symbol
    LSP_SYMBOL_CLASS = 5,          ///< Class symbol
    LSP_SYMBOL_METHOD = 6,         ///< Method symbol
    LSP_SYMBOL_PROPERTY = 7,       ///< Property symbol
    LSP_SYMBOL_FIELD = 8,          ///< Field symbol
    LSP_SYMBOL_CONSTRUCTOR = 9,    ///< Constructor symbol
    LSP_SYMBOL_ENUM = 10,          ///< Enumeration symbol
    LSP_SYMBOL_INTERFACE = 11,     ///< Interface symbol
    LSP_SYMBOL_FUNCTION = 12,      ///< Function symbol
    LSP_SYMBOL_VARIABLE = 13,      ///< Variable symbol
    LSP_SYMBOL_CONSTANT = 14,      ///< Constant symbol
    LSP_SYMBOL_STRING = 15,        ///< String symbol
    LSP_SYMBOL_NUMBER = 16,        ///< Number symbol
    LSP_SYMBOL_BOOLEAN = 17,       ///< Boolean symbol
    LSP_SYMBOL_ARRAY = 18,         ///< Array symbol
    LSP_SYMBOL_OBJECT = 19,        ///< Object symbol
    LSP_SYMBOL_KEY = 20,           ///< Key symbol
    LSP_SYMBOL_NULL = 21,          ///< Null symbol
    LSP_SYMBOL_ENUM_MEMBER = 22,   ///< Enumeration member symbol
    LSP_SYMBOL_STRUCT = 23,        ///< Structure symbol
    LSP_SYMBOL_EVENT = 24,         ///< Event symbol
    LSP_SYMBOL_OPERATOR = 25,      ///< Operator symbol
    LSP_SYMBOL_TYPE_PARAMETER = 26 ///< Type parameter symbol
};

/// \ingroup lsp_dynamic_types
/// \brief Dynamic value type enumeration
///
/// Defines all possible types that can be stored in dynamic values.
/// This enumeration is used as a discriminant in the tagged union.
///
enum lsp_dyn_kind {
    LSP_DYN_NULL = 0,     ///< Null value
    LSP_DYN_BOOLEAN,      ///< Boolean value
    LSP_DYN_INTEGER,      ///< Signed 64-bit integer
    LSP_DYN_UINTEGER,     ///< Unsigned 64-bit integer
    LSP_DYN_DECIMAL,      ///< Textual decimal representation (lossless)
    LSP_DYN_STRING,       ///< String value
    LSP_DYN_ARRAY,        ///< Array of dynamic values
    LSP_DYN_OBJECT        ///< Object with string keys and dynamic values
};


enum lsp_method_type {
    LSP_METHOD_INVALID = 0,
    LSP_METHOD_CALL_HIERARCHY_INCOMING_CALLS,
    LSP_METHOD_CALL_HIERARCHY_OUTGOING_CALLS,
    LSP_METHOD_CLIENT_REGISTER_CAPABILITY,
    LSP_METHOD_CLIENT_UNREGISTER_CAPABILITY,
    LSP_METHOD_CODE_ACTION_RESOLVE,
    LSP_METHOD_COMPLETION_ITEM_RESOLVE,
    LSP_METHOD_DOCUMENT_LINK_RESOLVE,
    LSP_METHOD_CANCEL_REQUEST_NOTIFICATION,
    LSP_METHOD_LOG_TRACE_NOTIFICATION,
    LSP_METHOD_PROGRESS_NOTIFICATION,
    LSP_METHOD_SET_TRACE_NOTIFICATION,
    LSP_METHOD_EXIT,
    LSP_METHOD_INLAY_HINT_RESOLVE,
    LSP_METHOD_INITIALIZE,
    LSP_METHOD_INITIALIZED,
    LSP_METHOD_NOTEBOOK_DOCUMENT_DID_CHANGE,
    LSP_METHOD_NOTEBOOK_DOCUMENT_DID_CLOSE,
    LSP_METHOD_NOTEBOOK_DOCUMENT_DID_OPEN,
    LSP_METHOD_NOTEBOOK_DOCUMENT_DID_SAVE,
    LSP_METHOD_SHUTDOWN,
    LSP_METHOD_TELEMETRY_EVENT,
    LSP_METHOD_TEXT_DOCUMENT_CODE_ACTION,
    LSP_METHOD_TEXT_DOCUMENT_CODE_LENS,
    LSP_METHOD_TEXT_DOCUMENT_COLOR_PRESENTATION,
    LSP_METHOD_TEXT_DOCUMENT_COMPLETION,
    LSP_METHOD_TEXT_DOCUMENT_DECLARATION,
    LSP_METHOD_TEXT_DOCUMENT_DEFINITION,
    LSP_METHOD_TEXT_DOCUMENT_DIAGNOSTIC,
    LSP_METHOD_TEXT_DOCUMENT_DID_CHANGE,
    LSP_METHOD_TEXT_DOCUMENT_DID_CLOSE,
    LSP_METHOD_TEXT_DOCUMENT_DID_OPEN,
    LSP_METHOD_TEXT_DOCUMENT_DID_SAVE,
    LSP_METHOD_TEXT_DOCUMENT_DOCUMENT_COLOR,
    LSP_METHOD_TEXT_DOCUMENT_DOCUMENT_HIGHLIGHT,
    LSP_METHOD_TEXT_DOCUMENT_DOCUMENT_LINK,
    LSP_METHOD_TEXT_DOCUMENT_DOCUMENT_SYMBOL,
    LSP_METHOD_TEXT_DOCUMENT_FOLDING_RANGE,
    LSP_METHOD_TEXT_DOCUMENT_FORMATTING,
    LSP_METHOD_TEXT_DOCUMENT_HOVER,
    LSP_METHOD_TEXT_DOCUMENT_IMPLEMENTATION,
    LSP_METHOD_TEXT_DOCUMENT_INLAY_HINT,
    LSP_METHOD_TEXT_DOCUMENT_INLINE_VALUE,
    LSP_METHOD_TEXT_DOCUMENT_INLINE_COMPLETION,
    LSP_METHOD_TEXT_DOCUMENT_LINKED_EDITING_RANGE,
    LSP_METHOD_TEXT_DOCUMENT_MONIKER,
    LSP_METHOD_TEXT_DOCUMENT_ON_TYPE_FORMATTING,
    LSP_METHOD_TEXT_DOCUMENT_PREPARE_CALL_HIERARCHY,
    LSP_METHOD_TEXT_DOCUMENT_PREPARE_RENAME,
    LSP_METHOD_TEXT_DOCUMENT_PREPARE_TYPE_HIERARCHY,
    LSP_METHOD_TEXT_DOCUMENT_PUBLISH_DIAGNOSTICS,
    LSP_METHOD_TEXT_DOCUMENT_RANGE_FORMATTING,
    LSP_METHOD_TEXT_DOCUMENT_RANGES_FORMATTING,
    LSP_METHOD_TEXT_DOCUMENT_REFERENCES,
    LSP_METHOD_TEXT_DOCUMENT_RENAME,
    LSP_METHOD_TEXT_DOCUMENT_SELECTION_RANGE,
    LSP_METHOD_TEXT_DOCUMENT_SEMANTIC_TOKENS_FULL,
    LSP_METHOD_TEXT_DOCUMENT_SEMANTIC_TOKENS_FULL_DELTA,
    LSP_METHOD_TEXT_DOCUMENT_SEMANTIC_TOKENS_RANGE,
    LSP_METHOD_TEXT_DOCUMENT_SIGNATURE_HELP,
    LSP_METHOD_TEXT_DOCUMENT_TYPE_DEFINITION,
    LSP_METHOD_TEXT_DOCUMENT_WILL_SAVE,
    LSP_METHOD_TEXT_DOCUMENT_WILL_SAVE_WAIT_UNTIL,
    LSP_METHOD_TYPE_HIERARCHY_SUBTYPES,
    LSP_METHOD_TYPE_HIERARCHY_SUPERTYPES,
    LSP_METHOD_WINDOW_LOG_MESSAGE,
    LSP_METHOD_WINDOW_SHOW_DOCUMENT,
    LSP_METHOD_WINDOW_SHOW_MESSAGE,
    LSP_METHOD_WINDOW_SHOW_MESSAGE_REQUEST,
    LSP_METHOD_WINDOW_WORK_DONE_PROGRESS_CANCEL,
    LSP_METHOD_WINDOW_WORK_DONE_PROGRESS_CREATE,
    LSP_METHOD_WORKSPACE_APPLY_EDIT,
    LSP_METHOD_WORKSPACE_CODE_LENS_REFRESH,
    LSP_METHOD_WORKSPACE_CONFIGURATION,
    LSP_METHOD_WORKSPACE_DIAGNOSTIC,
    LSP_METHOD_WORKSPACE_DIAGNOSTIC_REFRESH,
    LSP_METHOD_WORKSPACE_DID_CHANGE_CONFIGURATION,
    LSP_METHOD_WORKSPACE_DID_CHANGE_WATCHED_FILES,
    LSP_METHOD_WORKSPACE_DID_CHANGE_WORKSPACE_FOLDERS,
    LSP_METHOD_WORKSPACE_DID_CREATE_FILES,
    LSP_METHOD_WORKSPACE_DID_DELETE_FILES,
    LSP_METHOD_WORKSPACE_DID_RENAME_FILES,
    LSP_METHOD_WORKSPACE_EXECUTE_COMMAND,
    LSP_METHOD_WORKSPACE_FOLDING_RANGE_REFRESH,
    LSP_METHOD_WORKSPACE_INLAY_HINT_REFRESH,
    LSP_METHOD_WORKSPACE_INLINE_VALUE_REFRESH,
    LSP_METHOD_WORKSPACE_SEMANTIC_TOKENS_REFRESH,
    LSP_METHOD_WORKSPACE_SYMBOL,
    LSP_METHOD_WORKSPACE_SYMBOL_RESOLVE,
    LSP_METHOD_WORKSPACE_WILL_CREATE_FILES,
    LSP_METHOD_WORKSPACE_WILL_DELETE_FILES,
    LSP_METHOD_WORKSPACE_WILL_RENAME_FILES,
    LSP_METHOD_WORKSPACE_WORKSPACE_FOLDERS,

    LSP_METHOD_COUNT
};



// Intentionally left minimal; concrete message groups live in the
// other split headers. Keep any cross-cutting base declarations here if needed.

// ======================================================================================================================
// BASIC TYPES (no dependencies)
// ======================================================================================================================

/// \ingroup lsp_base_types
/// \brief URI type for document and resource identification
///
/// Represents a Uniform Resource Identifier as defined in RFC 3986.
/// Used for identifying documents, resources, and other entities
/// within the LSP protocol.
///
/// \see https://tools.ietf.org/html/rfc3986
struct lsp_uri {
    const char *chars;  ///< URI string data
};

/// \ingroup lsp_base_types
/// \brief Position in a text document (zero-based)
///
/// Represents a specific position within a text document using
/// line and character coordinates. Both line and character indices
/// are zero-based.
struct lsp_position {
    int line;      ///< Line number (0-based)
    int character; ///< Character position within the line (0-based)
};
 
/// \ingroup lsp_base_types
/// \brief URI type for document and resource identification
///
/// Represents a Uniform Resource Identifier as defined in RFC 3986.
/// Used for identifying documents, resources, and other entities
/// within the LSP protocol.
///
// lsp_client_info is defined after optional types since it uses lsp_opt_string

struct lsp_string {
    const char* chars;
    int char_len;
};

/// \ingroup lsp_base_types
/// \brief Range within a text document
///
/// Represents a contiguous range of text within a document,
/// defined by start and end positions. The range is inclusive
/// of the start position and exclusive of the end position.
struct lsp_range {
    struct lsp_position start;  ///< Start position of the range
    struct lsp_position end;    ///< End position of the range (exclusive)
};

struct lsp_location
{
    struct lsp_uri   document_uri;
	struct lsp_range range;
};

/// \ingroup lsp_base_types
/// \brief Generic singly-linked list
///
/// Template structure for representing linked lists of items.
/// Used throughout the LSP protocol for variable-length arrays
/// and collections of items.
///
/// \tparam T The type of items stored in the list
template <typename T>
struct lsp_list {
    T item;                           ///< Current item in the list
    struct lsp_list<T> *next;         ///< Pointer to next item (null for last item)
};

/// \ingroup lsp_base_types
/// \brief Message header for all LSP protocol messages
/// \details Standard header that prefixes all LSP messages in memory.
///Provides timing information, reference counting, and message
///correlation data for the garbage-collected message store.
///
struct lsp_msg_hdr {
    AkU64                     timestamp_nanos; ///< Creation timestamp in nanoseconds
    AkU64                     op_tag;          ///< Operation tag combining method and interaction
    AkU64                     tag_id;          ///< Unique tag for message correlation
    std::atomic<AkU32>        refcount;        ///< Atomic reference count for memory management
};

/// \file lsp_types.hpp
/// \brief LSP Protocol Core Type Definitions
///
/// This file contains the fundamental type definitions used throughout the LSP protocol.
/// Types are organized in topological order (dependencies first) and include comprehensive
/// Doxygen documentation. Types that are used multiple times across different message
/// structures are kept here, while single-use types are defined in their respective
/// message files.
///
/// \see https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/
/// \since 3.17.0




/// \defgroup lsp_dynamic_types Dynamic Types
/// \brief JSON-like dynamic value system for LSP protocol
/// 
/// Provides a type-safe way to represent JSON-like values without using
/// floating-point numbers (doubles). Uses discriminated unions and
/// tagged types for memory safety and performance.
/// 

struct lsp_dyn_decimal {
    lsp_string repr; // textual, lossless
};

struct lsp_dyn; // fwd

struct lsp_dyn_array {
    struct lsp_list<struct lsp_dyn *> *head;
    int count;
};

struct lsp_dyn_obj_entry {
    lsp_string key;
    struct lsp_dyn *value;
    struct lsp_dyn_obj_entry *next;
};

struct lsp_dyn_object {
    struct lsp_dyn_obj_entry **buckets;
    int capacity;
    int size;
};

struct lsp_dyn {
    int kind; // lsp_dyn_kind
    union {
        bool as_bool;
        long long as_i64;
        unsigned long long as_u64;
        lsp_dyn_decimal as_decimal;
        lsp_string as_string;
        lsp_dyn_array as_array;
        lsp_dyn_object as_object;
    } value;
};

// moved to lspd_language.hpp

/// \defgroup lsp_optional_types Optional Types
/// \brief Tagged optional value types
///
/// Provides discriminated union types for representing optional values
/// in a type-safe manner. Each optional type uses a discriminant to
/// indicate whether a value is present or absent.

/// \ingroup lsp_optional_types
/// \brief Optional value discriminant
///
/// Used as a discriminant in optional types to indicate whether
/// a value is present (SOME) or absent (NONE).
enum lsp_opt_kind {
    LSP_OPT_NONE = 0,  ///< Value is absent
    LSP_OPT_SOME = 1   ///< Value is present
};

/// \ingroup lsp_optional_types
/// \brief Optional string value
///
/// Represents an optional string that may or may not be present.
/// Uses discriminated union pattern for type safety.
struct lsp_opt_string {
    int        kind;  ///< lsp_opt_kind discriminant
    lsp_string value; ///< String value (valid iff kind == LSP_OPT_SOME)
};

/// \ingroup lsp_optional_types
/// \brief Optional unsigned integer value
/// 
/// Represents an optional unsigned integer that may or may not be present.
/// Uses discriminated union pattern for type safety.
/// 
struct lsp_opt_uinteger {
    int          kind;  ///< lsp_opt_kind discriminant
    unsigned int value; ///< Integer value (valid iff kind == LSP_OPT_SOME)
};

/// \ingroup lsp_optional_types
/// \brief Optional boolean value
/// 
/// Tri-state boolean type representing true, false, or null/absent.
/// Uses a single character to represent all three states efficiently.
/// 
using lsp_opt_bool = char;

/// \ingroup lsp_optional_types
/// \brief Optional boolean value constants
/// 
/// Defines the three possible states for lsp_opt_bool values.
/// 
enum lsp_opt_bool_type {
    LSP_BOOL_NULL  = '\0',  ///< Null/absent value
    LSP_BOOL_TRUE  = 't',   ///< Boolean true
    LSP_BOOL_FALSE = 'f'    ///< Boolean false
};

/// \ingroup lsp_base_types
/// \brief Client information supplied during initialization
///
/// Identifies the language client connecting to the server. Used for
/// capability negotiation and diagnostics.
struct lsp_client_info {
    struct lsp_string     name;    ///< Human-readable client name
    struct lsp_opt_string version; ///< Optional client version
};

/// \ingroup lsp_document_types
/// \brief Text edit operation
///
/// Represents a single text edit operation that can be applied
/// to modify document content.
struct lsp_text_edit {
    lsp_range range;      ///< Range to replace
    lsp_string new_text;  ///< Text to insert
};

/// \ingroup lsp_document_types
/// \brief List of text edits
///
/// Represents either a list of text edits or null.
/// Used for results that may contain multiple edits or no edits.
struct lsp_text_edit_list {
    int kind;  ///< LSP_OPT_NONE (null) or LSP_OPT_SOME (array)
    struct lsp_list<lsp_text_edit>* head;  ///< Head of edit list (valid if kind == SOME)
    int count; ///< Number of edits (valid if kind == SOME)
};


/// \ingroup lsp_base_types
/// \brief JSON-RPC error result
/// 
/// Represents an error response from a JSON-RPC request.
/// Contains error code and optional error message.
/// 
struct lsp_error_result {
    int code;                        ///< Error code
    struct lsp_opt_string message;   ///< Optional error message
};

// moved to lspd_window.hpp


// moved to lspd_workspace.hpp

// moved to lspd_window.hpp

// moved to lspd_window.hpp

/// \ingroup lsp_document_symbol_types
/// \brief Symbol tags list
///
/// Contains a list of tags that apply to a symbol.
struct lsp_symbol_tags {
    struct lsp_list<enum lsp_symbol_tag>* head;  ///< List of symbol tags
    int count;                                   ///< Number of tags
};
// ---------------------------------------------------------------------------------------------------------------------
// Completion types
// ---------------------------------------------------------------------------------------------------------------------


/// Completion item kind enumeration
// moved to lspd_language.hpp

/// \ingroup lsp_document_types
/// \brief Text document identifier
///
/// Uniquely identifies a text document using its URI.
/// Used in operations that don't require version information.
struct lsp_text_document_identifier {
    lsp_uri uri;  ///< Document URI
};

/// \ingroup lsp_document_types
/// \brief Complete text document information
/// \details Represents the full content and metadata of a text document
/// as provided in the didOpen notification.
struct lsp_text_document_item {
    lsp_uri    uri;          ///< Document URI
    lsp_string language_id;  ///< Language identifier (e.g., "cpp", "python")
    int        version;      ///< Document version number
    lsp_string text;         ///< Full document text content
};

/// Linked editing ranges
struct lsp_linked_editing_ranges {
    struct lsp_list<lsp_range>* ranges;
    lsp_opt_string              word_pattern; // regex pattern for word matching
};

struct lsp_versioned_text_document_id  {
    struct lsp_uri uri;
    int            version;
};

/// Moniker kind enumeration
enum lsp_moniker_kind {
    LSP_MONIKER_IMPORT = 0,
    LSP_MONIKER_EXPORT,
    LSP_MONIKER_LOCAL
};

/// Uniqueness level enumeration
enum lsp_uniqueness_level {
    LSP_UNIQUENESS_DOCUMENT = 0,
    LSP_UNIQUENESS_PROJECT,
    LSP_UNIQUENESS_GROUP,
    LSP_UNIQUENESS_SCHEME,
    LSP_UNIQUENESS_GLOBAL
};

/// Moniker structure
struct lsp_moniker {
    lsp_string                scheme;
    lsp_string                identifier;
    enum lsp_uniqueness_level unique;
    enum lsp_moniker_kind     kind;
};

/// Moniker list
struct lsp_moniker_list {
    struct lsp_list<struct lsp_moniker>* head;
    int                                  count;
};


/// Inline value union
enum lsp_inline_value_kind {
    LSP_INLINE_VALUE_TEXT = 0,
    LSP_INLINE_VALUE_VARIABLE_LOOKUP,
    LSP_INLINE_VALUE_EVALUATABLE_EXPRESSION
};


// Placeholder structures for inline values
struct lsp_inline_value_text {
    struct lsp_range  range;
    lsp_string text;
};

struct lsp_inline_value_variable_lookup {
    struct lsp_range  range;
    lsp_string variable_name;
    bool       case_sensitive_lookup;
};

struct lsp_inline_value_evaluatable_expression {
    struct lsp_range  range;
    lsp_string expression;
};

struct lsp_inline_value {
    int kind; // lsp_inline_value_kind
    union {
        struct lsp_inline_value_text                   text;
        struct lsp_inline_value_variable_lookup        variable_lookup;
        struct lsp_inline_value_evaluatable_expression evaluatable_expression;
    } value;
};

/// Inline value list
struct lsp_inline_value_list {
    struct lsp_list<struct lsp_inline_value>* head;
    int count;
};


/// Inlay hint kind enumeration
enum lsp_inlay_hint_kind {
    LSP_INLAY_HINT_TYPE = 1,
    LSP_INLAY_HINT_PARAMETER = 2
};

/// Inlay hint label part
struct lsp_inlay_hint_label_part {
    struct lsp_string      value;
    struct lsp_opt_string  tooltip;
    struct lsp_opt_string  location_present;
    struct lsp_location    location; // valid if present
    struct lsp_opt_string  command_present;
    struct lsp_dyn         command; // valid if present
};

/// Inlay hint label union
enum lsp_inlay_hint_label_kind {
    LSP_INLAY_HINT_LABEL_STRING = 0,
    LSP_INLAY_HINT_LABEL_PARTS
};

struct lsp_inlay_hint_label {
    int kind; // lsp_inlay_hint_label_kind
    union {
        lsp_string string_value;
        struct lsp_list<struct lsp_inlay_hint_label_part>* parts;
    } value;
};


/// Inlay hint structure
struct lsp_inlay_hint {
    lsp_position                position;
    struct lsp_inlay_hint_label label;
    enum lsp_inlay_hint_kind    kind;
    lsp_opt_string              text_edits_present;
    struct lsp_text_edit_list   text_edits; // valid if present
    lsp_opt_string              tooltip;
    lsp_opt_bool                padding_left;
    lsp_opt_bool                padding_right;
    lsp_opt_string              data;
};

/// Inlay hint list
struct lsp_inlay_hint_list {
    struct lsp_list<struct lsp_inlay_hint>* head;
    int count;
};

// ---------------------------------------------------------------------------------------------------------------------
// Diagnostic types (LSP 3.17)
// ---------------------------------------------------------------------------------------------------------------------

/// Document diagnostic parameters
struct lsp_document_diagnostic_params {
    struct lsp_text_document_identifier text_document;
    lsp_opt_string identifier;
    lsp_opt_string previous_result_id;
};

/// Document diagnostic report union
enum lsp_document_diagnostic_report_kind {
    LSP_DOCUMENT_DIAGNOSTIC_FULL = 0,
    LSP_DOCUMENT_DIAGNOSTIC_UNCHANGED
};

struct lsp_document_diagnostic_report {
    int kind; // lsp_document_diagnostic_report_kind
    union {
        struct {
            lsp_string result_id;
            struct lsp_list<struct lsp_diagnostic>* items;
        } full;
        struct {
            lsp_string result_id;
        } unchanged;
    } value;
};

struct lsp_workspace_diagnostic_work_done_progress {
    // TODO: define work done progress token
};

/// Workspace diagnostic parameters
struct lsp_workspace_diagnostic_params {
    lsp_opt_string identifier;
    lsp_opt_string previous_result_id;
    struct lsp_workspace_diagnostic_work_done_progress work_done_token; // TODO: define
};

/// Workspace diagnostic report
struct lsp_workspace_diagnostic_report {
    struct lsp_list<struct lsp_workspace_document_diagnostic_report>* items;
};

// Placeholder for workspace diagnostic structures
struct lsp_workspace_document_diagnostic_report {
    lsp_uri uri;
    int     version;
    struct lsp_document_diagnostic_report report;
};


// ---------------------------------------------------------------------------------------------------------------------
// Missing LSP 3.17 types for remaining messages
// ---------------------------------------------------------------------------------------------------------------------

/// Text document edit structure
struct lsp_text_document_edit {
    struct lsp_text_document_identifier text_document;
    struct lsp_list<struct lsp_text_edit>* edits;
};

// /// Implementation result (same as definition result)
// typedef lsp_definition_result lsp_implementation_result;
// /// Declaration result (same as definition result)
// typedef lsp_declaration_result lsp_declaration_result;

/// Workspace folders request parameters
struct lsp_workspace_folders_request_params {
    // Empty - no parameters
};

/// Workspace folders result
// struct lsp_workspace_folders_result {
//     int kind; // LSP_OPT_NONE or LSP_OPT_SOME
//     struct lsp_list<struct lsp_workspace_folder>* value; // valid if kind == SOME
// };

/// Workspace folders change event
struct lsp_workspace_folders_change_event {
    struct lsp_list<struct lsp_workspace_folder>* added;
    struct lsp_list<struct lsp_workspace_folder>* removed;
};

/// Did change workspace folders parameters
// struct lsp_did_change_workspace_folders_params {
//     struct lsp_workspace_folders_change_event event;
// };

/// Configuration item
// struct lsp_configuration_item {
//     lsp_opt_string scope_uri;
//     lsp_opt_string section;
// };

/// Configuration parameters
// struct lsp_configuration_params {
//     struct lsp_list<struct lsp_configuration_item>* items;
// };

/// Configuration result
// struct lsp_configuration_result {
//     struct lsp_list<struct lsp_dyn>* items; // array of LSPAny
// };

/// Did change configuration parameters
struct lsp_did_change_configuration_params {
    struct lsp_dyn settings;
};

// /// Did change watched files parameters
// struct lsp_did_change_watched_files_params {
//     struct lsp_file_event_list changes;
// };

/// Publish diagnostics parameters
struct lsp_publish_diagnostics_params {
    struct lsp_uri uri;
    struct lsp_opt_string version;
    struct lsp_list<struct lsp_diagnostic>* diagnostics;
};

/// Window show message notification parameters
// struct lsp_show_message_params {
//     enum lsp_message_type type;
//     lsp_string message;
// };



/// Telemetry event parameters
struct lsp_telemetry_event_params {
    struct lsp_dyn data;
};

/// Set trace notification parameters
struct lsp_set_trace_params {
    lsp_string value;
};

/// Log trace notification parameters
struct lsp_log_trace_params {
    lsp_string     message;
    lsp_opt_string verbose;
};

/// Cancel request parameters
struct lsp_cancel_params {
    lsp_dyn id; // number | string
};

/// Progress parameters
struct lsp_progress_params {
    lsp_dyn        token; // ProgressToken
    struct lsp_dyn value; // LSPAny
};

/// Client register capability parameters
struct lsp_registration_params {
    struct lsp_list<struct lsp_registration>* registrations;
};

/// Client unregister capability parameters
struct lsp_unregisteration_params {
    struct lsp_list<struct lsp_unregisteration>* unregisterations;
};

// Placeholder structures
struct lsp_registration {
    lsp_string     id;
    lsp_string     method;
    struct lsp_dyn register_options;
};

struct lsp_unregisteration {
    lsp_string id;
    lsp_string method;
};

// ---------------------------------------------------------------------------------------------------------------------
// Work done progress types
// ---------------------------------------------------------------------------------------------------------------------

// /// Work done progress kind enumeration
// enum lsp_work_done_progress_kind {
//     LSP_WORK_DONE_BEGIN = 0,
//     LSP_WORK_DONE_REPORT,
//     LSP_WORK_DONE_END
// };

// /// Work done progress begin
// struct lsp_work_done_progress_begin {
//     enum lsp_work_done_progress_kind kind;
//     lsp_string       title;
//     lsp_opt_bool     cancellable;
//     lsp_opt_string   message;
//     lsp_opt_uinteger percentage;
// };

// /// Work done progress report
// struct lsp_work_done_progress_report {
//     enum lsp_work_done_progress_kind kind;
//     lsp_opt_bool     cancellable;
//     lsp_opt_string   message;
//     lsp_opt_uinteger percentage;
// };

// /// Work done progress end
// struct lsp_work_done_progress_end {
//     enum lsp_work_done_progress_kind kind;
//     lsp_opt_string message;
// };

// /// Work done progress create parameters
// struct lsp_work_done_progress_create_params {
//     lsp_dyn token; // ProgressToken
// };

// /// Work done progress cancel parameters
// struct lsp_work_done_progress_cancel_params {
//     lsp_dyn token; // ProgressToken
// };

// ---------------------------------------------------------------------------------------------------------------------
// File operation types
// ---------------------------------------------------------------------------------------------------------------------

/// Create files parameters
struct lsp_create_files_params {
    struct lsp_list<struct lsp_file_create>* files;
};

/// Rename files parameters
struct lsp_rename_files_params {
    struct lsp_list<struct lsp_file_rename>* files;
};

/// Delete files parameters
struct lsp_delete_files_params {
    struct lsp_list<struct lsp_file_delete>* files;
};

/// File create
struct lsp_file_create {
    lsp_uri uri;
};

/// File rename
struct lsp_file_rename {
    lsp_string old_uri;
    lsp_string new_uri;
};

/// File delete
struct lsp_file_delete {
    lsp_uri uri;
};

// ---------------------------------------------------------------------------------------------------------------------
// Configuration and watched files types
// ---------------------------------------------------------------------------------------------------------------------

/// Did change configuration parameters
// struct lsp_did_change_configuration_params {
//     struct lsp_dyn settings;
// };

/// Did change watched files parameters
// struct lsp_did_change_watched_files_params {
//     struct lsp_file_event_list changes;
// };

// ---------------------------------------------------------------------------------------------------------------------
// Publish diagnostics types
// ---------------------------------------------------------------------------------------------------------------------

/// Publish diagnostics parameters
// struct lsp_publish_diagnostics_params {
//     lsp_uri uri;
//     lsp_opt_string version;
//     struct lsp_list<struct lsp_diagnostic>* diagnostics;
// };

// ---------------------------------------------------------------------------------------------------------------------
// Log message types
// ---------------------------------------------------------------------------------------------------------------------

/// Log message parameters
// struct lsp_log_message_params {
//     enum lsp_message_type type;
//     lsp_string message;
// };

// ---------------------------------------------------------------------------------------------------------------------
// Notebook document types
// ---------------------------------------------------------------------------------------------------------------------
enum lsp_notebook_cell_kind {
    LSP_NOTEBOOK_CELL_MARKUP = 1,
    LSP_NOTEBOOK_CELL_CODE = 2
};

struct lsp_notebook_document_sync_registration_options {
    bool save;
};


// Placeholder structures for notebook documents
struct lsp_notebook_document {
    lsp_uri uri;
    lsp_string notebook_type;
    int version;
    bool is_dirty;
    struct lsp_list<struct lsp_notebook_cell>* cells;
};

struct lsp_notebook_document_identifier {
    lsp_uri uri;
};

struct lsp_versioned_notebook_document_identifier {
    int version;
    lsp_uri uri;
};

/// Did open notebook document parameters
struct lsp_did_open_notebook_document_params {
    struct lsp_notebook_document notebook_document;
    struct lsp_notebook_document_sync_registration_options cell_text_documents;
};

/// Did change notebook document parameters
struct lsp_did_change_notebook_document_params {
    struct lsp_versioned_notebook_document_identifier notebook_document;
    struct lsp_list<struct lsp_notebook_cell>* change; // TODO: define notebook cell changes
};

/// Did save notebook document parameters
struct lsp_did_save_notebook_document_params {
    struct lsp_notebook_document_identifier notebook_document;
};

/// Did close notebook document parameters
struct lsp_did_close_notebook_document_params {
    struct lsp_notebook_document_identifier notebook_document;
    bool save;
};





struct lsp_notebook_cell {
    enum lsp_notebook_cell_kind kind;
    struct lsp_list<struct lsp_text_document_item>* documents;
};



/// Code action result
struct lsp_code_action_result {
    int kind; // LSP_OPT_NONE or LSP_OPT_SOME
    struct lsp_list<struct lsp_code_action>* value; // valid if kind == SOME
};

/// Code action disabled reason
struct lsp_code_action_disabled {
    lsp_string reason;
};




/// \ingroup lsp_document_symbol_types
/// \brief Symbol information (flat)
///
/// Flat representation of a symbol with location information.
/// Used for workspace-wide symbol operations.
struct lsp_symbol_information {
    lsp_string name;                ///< Symbol name
    enum lsp_symbol_kind kind;      ///< Symbol kind
    struct lsp_symbol_tags tags;    ///< Symbol tags
    lsp_opt_string container_name;  ///< Optional container name
    struct lsp_location location;   ///< Symbol location
};

struct lsp_text_document_position_params  {
    struct lsp_text_document_identifier text_document;
    struct lsp_position                 position;
};

