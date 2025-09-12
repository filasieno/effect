#pragma once

#include "lsp_basic.hpp" // IWYU pragma: keep
#include "lsp_dyn.hpp"   // IWYU pragma: keep

/// \defgroup lsp_language_features Language Features
/// \brief Language-intelligence features and related messages
/// \see https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/

// ======================================================================================================================
// SHARED RESULT TYPES (used by multiple features)
// ======================================================================================================================

/// \ingroup lsp_document_types
/// \brief Definition request result
///
/// Union type representing the result of a definition request.
/// Can contain a single location or a list of location links.
struct lsp_definition_result {
    int kind;  ///< lsp_definition_result_kind discriminant
    union {
        struct lsp_location *as_location;                               ///< Single location (if kind == LSP_DEF_LOCATION)
        struct lsp_list<struct lsp_location_link *> *as_location_link_list;  ///< Location links (if kind == LSP_DEF_LOCATION_LINK_LIST)
    } value;
};

/// \ingroup lsp_document_types
/// \brief Declaration request result
///
/// Declaration results use the same structure as definition results.
/// This type alias provides clarity for declaration-specific operations.
struct lsp_declaration_result {
    int kind;  ///< lsp_definition_result_kind discriminant (same as definition)
    union {
        struct lsp_location *as_location;                               ///< Single location (if kind == LSP_DEF_LOCATION)
        struct lsp_list<struct lsp_location_link *> *as_location_link_list;  ///< Location links (if kind == LSP_DEF_LOCATION_LINK_LIST)
    } value;
};

// ======================================================================================================================
// DECLARATION & DEFINITION FEATURES
// ======================================================================================================================

/// \ingroup lsp_language_features
/// \section textDocument_declaration Go to Declaration
/// \brief Request/Response/Error for textDocument/declaration

struct lsp_text_document_declaration_request {
    struct lsp_msg_hdr hdr;
    struct lsp_text_document_position_params params;
};

struct lsp_text_document_declaration_response {
    struct lsp_msg_hdr hdr;
    struct lsp_declaration_result result;
};

struct lsp_text_document_declaration_error_result {
    struct lsp_msg_hdr hdr;
    struct lsp_error_result error;
};

static struct lsp_text_document_declaration_request*      lsp_init_text_document_declaration_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_declaration_response*     lsp_init_text_document_declaration_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_declaration_error_result* lsp_init_text_document_declaration_error_result(void *mem, AkU64 tag_id) noexcept;

/// \ingroup lsp_language_features
/// \section textDocument_definition Go to Definition
/// \brief Request/Partial/Response/Error for textDocument/definition

struct lsp_text_document_definition_request
{
    struct lsp_msg_hdr hdr;
    struct lsp_text_document_position_params params;
};

struct lsp_text_document_definition_partial_response
{
    struct lsp_msg_hdr hdr;
};

struct lsp_text_document_definition_response
{
    struct lsp_msg_hdr hdr;
    struct lsp_definition_result result;
};

struct lsp_text_document_definition_error_result
{
    struct lsp_msg_hdr hdr;
    struct lsp_error_result error;
};

static struct lsp_text_document_definition_request*          lsp_init_text_document_definition_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_definition_partial_response* lsp_init_text_document_definition_partial_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_definition_response*         lsp_init_text_document_definition_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_definition_error_result*     lsp_init_text_document_definition_error_result(void *mem, AkU64 tag_id) noexcept;

// ======================================================================================================================
// TYPE DEFINITION & IMPLEMENTATION FEATURES
// ======================================================================================================================

/// \ingroup lsp_language_features
/// \section textDocument_typeDefinition Go to Type Definition

/// \ingroup lsp_language_features
/// \section textDocument_implementation Go to Implementation

// ======================================================================================================================
// REFERENCE FEATURES
// ======================================================================================================================

/// \ingroup lsp_language_features
/// \section textDocument_references Find References

// ======================================================================================================================
// HIERARCHY FEATURES (Call & Type)
// ======================================================================================================================

/// \ingroup lsp_language_features
/// \section textDocument_prepareCallHierarchy Prepare Call Hierarchy

/// \ingroup lsp_language_features
/// \section callHierarchy_incomingCalls Call Hierarchy Incoming Calls

/// \ingroup lsp_language_features
/// \section callHierarchy_outgoingCalls Call Hierarchy Outgoing Calls

/// \ingroup lsp_language_features
/// \section textDocument_prepareTypeHierarchy Prepare Type Hierarchy

/// \ingroup lsp_language_features
/// \section typeHierarchy_supertypes Type Hierarchy Super Types

/// \ingroup lsp_language_features
/// \section typeHierarchy_subtypes Type Hierarchy Sub Types

// ======================================================================================================================
// DOCUMENT ANALYSIS FEATURES
// ======================================================================================================================

/// \ingroup lsp_language_features
/// \section textDocument_documentHighlight Document Highlight

/// \ingroup lsp_language_features
/// \section textDocument_documentLink Document Link

/// \ingroup lsp_language_features
/// \section documentLink_resolve Document Link Resolve

/// \ingroup lsp_language_features
/// \section textDocument_hover Hover

/// \ingroup lsp_language_features
/// \section textDocument_documentSymbol Document Symbols

// ======================================================================================================================
// CODE LENS & FOLDING FEATURES
// ======================================================================================================================

/// \ingroup lsp_language_features
/// \section textDocument_codeLens Code Lens

/// \ingroup lsp_language_features
/// \section codeLens_refresh Code Lens Refresh

/// \ingroup lsp_language_features
/// \section textDocument_foldingRange Folding Range

/// \ingroup lsp_language_features
/// \section textDocument_selectionRange Selection Range

// ======================================================================================================================
// SEMANTIC FEATURES
// ======================================================================================================================

/// \ingroup lsp_language_features
/// \section textDocument_semanticTokens Semantic Tokens

/// \ingroup lsp_language_features
/// \section textDocument_inlineValue Inline Value

/// \ingroup lsp_language_features
/// \section workspace_inlineValue_refresh Inline Value Refresh

/// \ingroup lsp_language_features
/// \section textDocument_inlayHint Inlay Hint

/// \ingroup lsp_language_features
/// \section inlayHint_resolve Inlay Hint Resolve

/// \ingroup lsp_language_features
/// \section workspace_inlayHint_refresh Inlay Hint Refresh

/// \ingroup lsp_language_features
/// \section textDocument_moniker Moniker

// ======================================================================================================================
// COMPLETION FEATURES
// ======================================================================================================================

/// \ingroup lsp_language_features
/// \section textDocument_completion Completion Proposals

/// \ingroup lsp_language_features
/// \section completionItem_resolve Completion Item Resolve

// ======================================================================================================================
// DIAGNOSTIC FEATURES
// ======================================================================================================================

/// \ingroup lsp_language_features
/// \section textDocument_publishDiagnostics Publish Diagnostics

/// \ingroup lsp_language_features
/// \section textDocument_pullDiagnostics Pull Diagnostics

// ======================================================================================================================
// SIGNATURE HELP & CODE ACTION FEATURES
// ======================================================================================================================

/// \ingroup lsp_language_features
/// \section textDocument_signatureHelp Signature Help

/// \ingroup lsp_language_features
/// \section textDocument_codeAction Code Action

/// \ingroup lsp_language_features
/// \section codeAction_resolve Code Action Resolve

// ======================================================================================================================
// COLOR FEATURES
// ======================================================================================================================

/// \ingroup lsp_language_features
/// \section textDocument_documentColor Document Color

/// \ingroup lsp_language_features
/// \section textDocument_colorPresentation Color Presentation

// ======================================================================================================================
// FORMATTING FEATURES
// ======================================================================================================================

/// \ingroup lsp_language_features
/// \section textDocument_formatting Formatting

/// \ingroup lsp_language_features
/// \section textDocument_rangeFormatting Range Formatting

/// \ingroup lsp_language_features
/// \section textDocument_onTypeFormatting On type Formatting

// ======================================================================================================================
// RENAME & EDITING FEATURES
// ======================================================================================================================

/// \ingroup lsp_language_features
/// \section textDocument_rename Rename

/// \ingroup lsp_language_features
/// \section textDocument_prepareRename Prepare Rename

/// \ingroup lsp_language_features
/// \section textDocument_linkedEditingRange Linked Editing Range

/// \defgroup lsp_completion_types Completion Types
/// \brief Types for completion requests and responses
///
/// Types used specifically for completion operations.

/// \ingroup lsp_completion_types
/// \brief Completion trigger kind
///
/// Specifies what triggered a completion request.
/// Used to determine completion behavior and filtering.
enum lsp_completion_trigger_kind {
    LSP_COMPLETION_TRIGGER_INVOKED = 1,                      ///< Manually invoked
    LSP_COMPLETION_TRIGGER_CHARACTER = 2,                    ///< Trigger character typed
    LSP_COMPLETION_TRIGGER_FOR_INCOMPLETE_COMPLETIONS = 3    ///< For incomplete completions
};

struct lsp_completion_list {

};

struct lsp_completion_result {

};

/// \ingroup lsp_completion_types
/// \brief Completion context information
///
/// Additional context information about a completion request.
/// Provides details about what triggered the completion.
struct lsp_completion_context {
    enum lsp_completion_trigger_kind trigger_kind;  ///< What triggered completion
    lsp_opt_string trigger_character;               ///< Trigger character (if applicable)
};

/// \ingroup lsp_completion_types
/// \brief Completion request parameters
///
/// Parameters for a completion request, including document position
/// and completion context information.
struct lsp_completion_params {
    struct lsp_text_document_position_params text_document_position;  ///< Document and position
    struct lsp_completion_context context;                           ///< Completion context
};

/// The `textDocument/completion` request is sent from the client to the server to compute completion items at a given cursor position.
/// The request's parameter is of type \ref lsp_completion_params and the response is of type \ref lsp_completion_item or \ref lsp_completion_list or a Thenable that resolves to such.
/// The request can delay the computation of the \ref lsp_completion_item.detail `detail` and \ref lsp_completion_item.documentation `documentation` properties to the `completionItem/resolve` request.
/// However, properties that are needed for the initial sorting and filtering, like `sortText`, `filterText`, `insertText`, and `textEdit`, must not be changed during resolve.
/// \since 3.0.0
struct lsp_text_document_completion_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Completion parameters including position and context.
    struct lsp_completion_params params;
};
/// \brief Partial completion result for streaming responses.
struct lsp_text_document_completion_partial_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Partial completion items.
    struct lsp_completion_list partial;
};
/// \brief Final completion result.
struct lsp_text_document_completion_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Completion result (list or items array).
    struct lsp_completion_result result;
};
/// \brief Error result for completion request.
struct lsp_text_document_completion_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_text_document_completion_request*          lsp_init_text_document_completion_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_completion_partial_response* lsp_init_text_document_completion_partial_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_completion_response*         lsp_init_text_document_completion_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_completion_error_result*     lsp_init_text_document_completion_error_result(void *mem, AkU64 tag_id) noexcept;

/// The `completionItem/resolve` request is sent from the client to the server to resolve additional information for a given completion item.
/// The request's parameter is of type \ref lsp_completion_item and the response is of type \ref lsp_completion_item or a Thenable that resolves to such.
/// This request is sent when a completion item is selected in the user interface and the client needs to resolve additional information about the item.
/// \ingroup lsp_completion_types
/// \brief Completion item kind enumeration
///
/// Defines the different kinds of completion items that can be provided.
/// Used to categorize completion suggestions for better user experience.
enum lsp_completion_item_kind {
    LSP_COMPLETION_TEXT = 1,           ///< Text completion
    LSP_COMPLETION_METHOD = 2,         ///< Method completion
    LSP_COMPLETION_FUNCTION = 3,       ///< Function completion
    LSP_COMPLETION_CONSTRUCTOR = 4,    ///< Constructor completion
    LSP_COMPLETION_FIELD = 5,          ///< Field completion
    LSP_COMPLETION_VARIABLE = 6,       ///< Variable completion
    LSP_COMPLETION_CLASS = 7,          ///< Class completion
    LSP_COMPLETION_INTERFACE = 8,      ///< Interface completion
    LSP_COMPLETION_MODULE = 9,         ///< Module completion
    LSP_COMPLETION_PROPERTY = 10,      ///< Property completion
    LSP_COMPLETION_UNIT = 11,          ///< Unit completion
    LSP_COMPLETION_VALUE = 12,         ///< Value completion
    LSP_COMPLETION_ENUM = 13,          ///< Enumeration completion
    LSP_COMPLETION_KEYWORD = 14,       ///< Keyword completion
    LSP_COMPLETION_SNIPPET = 15,       ///< Snippet completion
    LSP_COMPLETION_COLOR = 16,         ///< Color completion
    LSP_COMPLETION_FILE = 17,          ///< File completion
    LSP_COMPLETION_REFERENCE = 18,     ///< Reference completion
    LSP_COMPLETION_FOLDER = 19,        ///< Folder completion
    LSP_COMPLETION_ENUM_MEMBER = 20,   ///< Enumeration member completion
    LSP_COMPLETION_CONSTANT = 21,      ///< Constant completion
    LSP_COMPLETION_STRUCT = 22,        ///< Structure completion
    LSP_COMPLETION_EVENT = 23,         ///< Event completion
    LSP_COMPLETION_OPERATOR = 24,      ///< Operator completion
    LSP_COMPLETION_TYPE_PARAMETER = 25 ///< Type parameter completion
};
/// \ingroup lsp_completion_types
/// \brief Insert text format enumeration
///
/// Defines how the insert text for a completion item should be interpreted.
/// Determines whether the text is plain text or a snippet with placeholders.
enum lsp_insert_text_format {
    LSP_INSERT_TEXT_PLAIN_TEXT = 1,  ///< Plain text format
    LSP_INSERT_TEXT_SNIPPET = 2      ///< Snippet format with placeholders
};
/// \ingroup lsp_completion_types
/// \brief Completion item tag enumeration
///
/// Additional properties that can be attached to completion items.
enum lsp_completion_item_tag {
    LSP_COMPLETION_ITEM_DEPRECATED = 1  ///< Item is deprecated
};

/// \ingroup lsp_completion_types
/// \brief Completion item tags list
///
/// Contains a list of tags that apply to a completion item.
struct lsp_completion_item_tags {
    struct lsp_list<enum lsp_completion_item_tag>* head;  ///< List of completion item tags
    int count;                                            ///< Number of tags
};

/// \ingroup lsp_completion_types
/// \brief Text edit for completion item
///
/// Represents a text edit operation for a completion item.
/// Can be either a regular text edit or an insert/replace edit.
struct lsp_completion_item_text_edit {
    int kind;  ///< 0 = text edit, 1 = insert/replace edit
    union {
        struct lsp_text_edit text_edit;              ///< Regular text edit
        struct lsp_text_edit insert_replace_edit;    ///< Insert/replace text edit
    } value;
};

/// \ingroup lsp_completion_types
/// \brief Completion item structure
///
/// Represents a single completion suggestion with all its properties.
/// Contains label, kind, documentation, and editing information.
struct lsp_completion_item {
    struct lsp_string label;                                   ///< Completion label
    struct lsp_opt_string label_details;                        ///< Optional label details (since 3.17.0)
    enum lsp_completion_item_kind kind;                  ///< Completion item kind
    struct lsp_opt_string detail;                              ///< Optional detail text
    struct lsp_opt_string documentation;                        ///< Optional documentation
    lsp_opt_bool deprecated;                             ///< Whether item is deprecated
    struct lsp_completion_item_tags tags;                ///< Completion item tags (since 3.15.0)
    lsp_opt_bool preselect;                              ///< Whether to preselect this item
    struct lsp_opt_string sort_text;                            ///< Optional sort text
    struct lsp_opt_string filter_text;                          ///< Optional filter text
    struct lsp_opt_string insert_text;                          ///< Optional insert text
    enum lsp_insert_text_format insert_text_format;      ///< Insert text format
    struct lsp_opt_string text_edit_present;                    ///< Whether text edit is present
    struct lsp_completion_item_text_edit text_edit;      ///< Text edit (valid if present)
    struct lsp_text_edit_list additional_text_edits;     ///< Additional text edits
    struct lsp_opt_string commit_characters_present;            ///< Whether commit characters are present
    struct lsp_list<lsp_string>* commit_characters;      ///< Commit characters (valid if present)
    struct lsp_opt_string command_present;                      ///< Whether command is present
    struct lsp_dyn command;                              ///< Command to execute (valid if present)
    struct lsp_opt_string data;                                 ///< Optional completion data
};

/// \since 3.0.0
struct lsp_completion_item_resolve_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Completion item to resolve.
    struct lsp_completion_item params;
};
/// \brief Final resolved completion item.
struct lsp_completion_item_resolve_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Resolved completion item with additional details.
    struct lsp_completion_item result;
};
/// \brief Error result for completion item resolve.
struct lsp_completion_item_resolve_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_completion_item_resolve_request*      lsp_init_completion_item_resolve_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_completion_item_resolve_response*     lsp_init_completion_item_resolve_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_completion_item_resolve_error_result* lsp_init_completion_item_resolve_error_result(void *mem, AkU64 tag_id) noexcept;


/// \defgroup lsp_hover_types Hover Types
/// \brief Types for hover requests and responses
///
/// Types used specifically for hover operations.

/// \ingroup lsp_hover_types
/// \brief Hover contents union discriminant
///
/// Defines the possible types of hover content that can be displayed.
/// Used to determine whether the hover content is markup or a list of marked strings.
enum lsp_hover_contents_kind {
    LSP_HOVER_MARKUP = 0,           ///< Markup content (e.g., Markdown)
    LSP_HOVER_MARKED_STRING_LIST    ///< List of marked strings
};

/// \ingroup lsp_hover_types
/// \brief Marked string for hover content
///
/// Represents a string that can be either plain text or language-specific code.
/// Used in hover content to display formatted text with optional syntax highlighting.
struct lsp_marked_string {
    int kind;  ///< 0 = plain string, 1 = language-specific string
    union {
        lsp_string plain_string;    ///< Plain text string (when kind == 0)
        struct {
            lsp_string language;    ///< Language identifier for syntax highlighting
            lsp_string value;       ///< The actual string content
        } language_string;          ///< Language-specific string (when kind == 1)
    } value;
};

/// \ingroup lsp_hover_types
/// \brief Hover contents union
///
/// Represents the content that can be displayed in a hover popup.
/// Can be either markup content or a list of marked strings.
struct lsp_hover_contents {
    int kind;  ///< lsp_hover_contents_kind discriminant
    union {
        struct lsp_dyn markup_content;                         ///< Markup content (e.g., Markdown)
        struct lsp_list<struct lsp_marked_string>* marked_strings;  ///< List of marked strings
    } value;
};

/// \ingroup lsp_hover_types
/// \brief Hover result with optional range
///
/// Represents the result of a hover request, containing the content to display
/// and optionally the range of text that triggered the hover.
struct lsp_hover_result {
    int kind;  ///< LSP_OPT_NONE or LSP_OPT_SOME
    struct lsp_hover_contents value;    ///< Hover contents (valid if kind == SOME)
    struct lsp_opt_string range_present;       ///< Whether a range is specified
    struct lsp_range range;                    ///< Text range (valid if range_present)
};

/// The `textDocument/hover` request is sent from the client to the server to request hover information at a given text document position.
/// The request's parameter is of type \ref lsp_text_document_position_params and the response is of type \ref lsp_hover_result or a Thenable that resolves to such.
/// \since 3.0.0
struct lsp_text_document_hover_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Text document position for hover.
    struct lsp_text_document_position_params params;
};
/// \brief Final hover response.
struct lsp_text_document_hover_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Hover information or null.
    struct lsp_hover_result result;
};
/// \brief Error result for hover request.
struct lsp_text_document_hover_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_text_document_hover_request*      lsp_init_text_document_hover_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_hover_response*     lsp_init_text_document_hover_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_hover_error_result* lsp_init_text_document_hover_error_result(void *mem, AkU64 tag_id) noexcept;

/// \ingroup lsp_signature_help_types
/// \brief Signature help information
///
/// Contains all available signatures for a function call and tracks
/// the currently active signature and parameter.
struct lsp_signature_help {
    struct lsp_list<struct lsp_signature_information>* signatures;  ///< Available signatures
    lsp_opt_uinteger active_signature;                              ///< Active signature index
    lsp_opt_uinteger active_parameter;                              ///< Active parameter index
};

/// The `textDocument/signatureHelp` request is sent from the client to the server to request signature help at a given cursor position.
/// The request's parameter is of type \ref lsp_text_document_position_params and the response is of type \ref lsp_signature_help or a Thenable that resolves to such.
/// \since 3.0.0
struct lsp_text_document_signature_help_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Text document position for signature help.
    struct lsp_text_document_position_params params;
};

/// \brief Final signature help response.
struct lsp_text_document_signature_help_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Signature help information.
    struct lsp_signature_help result;
};

/// \brief Error result for signature help request.
struct lsp_text_document_signature_help_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_text_document_signature_help_request*      lsp_init_text_document_signature_help_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_signature_help_response*     lsp_init_text_document_signature_help_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_signature_help_error_result* lsp_init_text_document_signature_help_error_result(void *mem, AkU64 tag_id) noexcept;

/// \brief Request to find all references to a symbol at a given position.
/// \details The `textDocument/references` request is sent from the client to
/// the server to resolve all locations where a symbol at a given text document
/// position is referenced. This includes the symbol's definition, declarations,
/// and all usage locations across the entire codebase.
///
/// \par Use Cases:
/// - "Find all references" functionality in IDEs
/// - Understanding symbol usage patterns
/// - Refactoring assistance and impact analysis
/// - Code navigation and exploration
///
/// \par Reference Types Included:
/// - Variable declarations and usages
/// - Function definitions and calls
/// - Class/type definitions and instantiations
/// - Import statements and module references
/// - Macro definitions and expansions (language-dependent)
///
/// \par Context Control:
/// The request can be configured to include or exclude certain types of
/// references based on the context flags in the parameters.
///
/// \param params Reference parameters including position and context settings
/// \result Array of Location objects or null if no references found
///
/// \since 3.0.0
/// \sa lsp_text_document_definition_request, lsp_text_document_declaration_request,
/// \ingroup lsp_reference_types
/// \brief Reference search context
///
/// Controls what types of references to include in the search results.
/// Determines whether declarations should be included along with references.
struct lsp_reference_context {
    bool include_declaration;  ///< Whether to include declaration in results
};

/// \ingroup lsp_reference_types
/// \brief Parameters for reference search requests
///
/// Contains the position in the text document where the symbol is located,
/// along with context flags that control what types of references to include.
struct lsp_reference_params {
    struct lsp_text_document_position_params text_document_position;  ///< Position to search from
    struct lsp_reference_context context;                             ///< Search context options
};

///     lsp_reference_params, lsp_locations_result
struct lsp_text_document_references_request {
    /// \brief Common message header with timing and correlation info.
    /// \details Standard LSP message metadata for request tracking and
    /// memory management.
    struct lsp_msg_hdr hdr;

    /// \brief Parameters specifying the reference search criteria.
    /// \details Contains the text document position where the symbol
    /// is located, along with context flags that control what types
    /// of references should be included in the search results.
    ///
    /// \sa lsp_reference_params
    struct lsp_reference_params params;
};

/// \brief Response containing all found symbol references.
/// \details Returns an array of Location objects representing all the
/// places where the symbol at the requested position is referenced.
/// Each location includes the file URI and the specific range within
/// that file where the reference occurs.
///
/// \par Result Structure:
/// - Empty array: Symbol exists but has no references
/// - Null: Symbol not found or references not supported
/// - Array of locations: All found reference locations
///
/// \par Location Details:
/// Each Location object contains:
/// - URI: The file where the reference occurs
/// - Range: The exact character range of the reference
///
/// \param result Array of reference locations or null
/// \result None - this is the final response
///
/// \ingroup lsp_reference_types
/// \brief Locations result (array or null)
///
/// Represents either a list of locations or null, used for reference
/// search results and other location-based operations.
struct lsp_locations_result {
    int kind;  ///< LSP_OPT_NONE (null) or LSP_OPT_SOME (array)
    struct lsp_list<struct lsp_location>* head;  ///< Location list (valid if kind == SOME)
    int count;  ///< Number of locations (valid if kind == SOME)
};

/// \since 3.0.0
/// \sa lsp_text_document_references_request, lsp_locations_result, lsp_location
struct lsp_text_document_references_response {
    /// \brief Common message header for response correlation.
    /// \details Links this response back to the original references request
    /// for proper message sequencing and error handling.
    struct lsp_msg_hdr hdr;

    /// \brief Complete list of symbol reference locations.
    /// \details Contains all locations where the requested symbol is
    /// referenced. Each location specifies the exact file and character
    /// range where the reference appears.
    ///
    /// \note Results may be empty if the symbol has no references,
    /// or null if the operation is not supported for the given symbol.
    ///
    /// \sa lsp_locations_result
    struct lsp_locations_result result;
};

/// \brief Error response for failed references request.
/// \details Sent when the server cannot process the references request,
/// typically due to parsing errors, symbol resolution failures, or
/// unsupported file types.
///
/// \par Common Error Conditions:
/// - Symbol not found at the specified position
/// - File parsing errors preventing symbol analysis
/// - Unsupported language or file type
/// - Server resource limitations
/// - Invalid position coordinates
///
/// \param error JSON-RPC error with specific failure details
/// \result None - operation failed
///
/// \since 3.0.0
/// \sa lsp_text_document_references_request, lsp_error_result
struct lsp_text_document_references_error_result {
    /// \brief Common message header for error correlation.
    /// \details Maintains the request-response linkage even for error cases.
    struct lsp_msg_hdr hdr;

    /// \brief Detailed error information.
    /// \details Contains the specific error code and message explaining
    /// why the references request could not be fulfilled.
    ///
    /// \sa lsp_error_result
    struct lsp_error_result error;
};
static struct lsp_text_document_references_request*      lsp_init_text_document_references_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_references_response*     lsp_init_text_document_references_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_references_error_result* lsp_init_text_document_references_error_result(void *mem, AkU64 tag_id) noexcept;

/// \brief Request to resolve the implementation location of a symbol.
/// \details The `textDocument/implementation` request is sent from the client to
/// the server to resolve the implementation location(s) of a symbol at a given
/// text document position. This is typically used to find the actual implementation
/// of an interface method or abstract method.
///
/// \par Use Cases:
/// - Finding concrete implementations of abstract methods
/// - Navigating from interface declarations to implementations
/// - Understanding polymorphic code structure
/// - Refactoring and impact analysis
///
/// \param params Text document position for implementation lookup
/// \result Array of Location objects or null if no implementations found
///
/// \since 3.6.0
/// \sa lsp_text_document_definition_request, lsp_text_document_declaration_request
/// \ingroup lsp_advanced
struct lsp_text_document_implementation_request {
    /// \brief Common message header with timing and correlation info.
    struct lsp_msg_hdr hdr;

    /// \brief Parameters specifying the implementation search criteria.
    /// \details Contains the text document position where the symbol
    /// is located for implementation resolution.
    struct lsp_text_document_position_params params;
};

/// \brief Response containing symbol implementation locations.
/// \details Returns an array of Location objects representing all the
/// places where the symbol at the requested position is implemented.
/// Each location includes the file URI and the specific range within
/// that file where the implementation occurs.
///
/// \param result Array of implementation locations or null
/// \result None - this is the final response
///
/// \since 3.6.0
/// \sa lsp_text_document_implementation_request, lsp_locations_result
/// \ingroup lsp_advanced
struct lsp_text_document_implementation_response {
    /// \brief Common message header for response correlation.
    struct lsp_msg_hdr hdr;

    /// \brief Complete list of implementation locations.
    /// \details Contains all locations where the requested symbol is
    /// implemented. Each location specifies the exact file and character
    /// range where the implementation appears.
    struct lsp_locations_result result;
};

/// \brief Error response for failed implementation request.
/// \details Sent when the server cannot process the implementation request,
/// typically due to parsing errors, symbol resolution failures, or
/// unsupported file types.
///
/// \param error JSON-RPC error with specific failure details
/// \result None - operation failed
///
/// \since 3.6.0
/// \sa lsp_text_document_implementation_request, lsp_error_result
/// \ingroup lsp_advanced
struct lsp_text_document_implementation_error_result {
    /// \brief Common message header for error correlation.
    struct lsp_msg_hdr hdr;

    /// \brief Detailed error information.
    /// \details Contains the specific error code and message explaining
    /// why the implementation request could not be fulfilled.
    struct lsp_error_result error;
};
static struct lsp_text_document_implementation_request*      lsp_init_text_document_implementation_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_implementation_response*     lsp_init_text_document_implementation_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_implementation_error_result* lsp_init_text_document_implementation_error_result(void *mem, AkU64 tag_id) noexcept;

/// \brief Request to resolve the type definition location of a symbol.
/// \details The `textDocument/typeDefinition` request is sent from the client to
/// the server to resolve the type definition location(s) of a symbol at a given
/// text document position. This is used to find where types are actually defined.
///
/// \par Use Cases:
/// - Finding type definitions for variables, parameters, and return types
/// - Navigating from usage to type declarations
/// - Understanding type hierarchies and relationships
///
/// \param params Text document position for type definition lookup
/// \result Array of Location objects or null if no type definitions found
///
/// \since 3.6.0
/// \sa lsp_text_document_definition_request, lsp_text_document_implementation_request
/// \ingroup lsp_advanced
struct lsp_text_document_type_definition_request {
    /// \brief Common message header with timing and correlation info.
    struct lsp_msg_hdr hdr;

    /// \brief Parameters specifying the type definition search criteria.
    /// \details Contains the text document position where the symbol
    /// is located for type definition resolution.
    struct lsp_text_document_position_params params;
};

/// \brief Response containing type definition locations.
/// \details Returns an array of Location objects representing all the
/// places where the type of the symbol at the requested position is defined.
/// Each location includes the file URI and the specific range within
/// that file where the type definition occurs.
///
/// \param result Array of type definition locations or null
/// \result None - this is the final response
///
/// \since 3.6.0
/// \sa lsp_text_document_type_definition_request, lsp_locations_result
/// \ingroup lsp_advanced
struct lsp_text_document_type_definition_response {
    /// \brief Common message header for response correlation.
    struct lsp_msg_hdr hdr;

    /// \brief Complete list of type definition locations.
    /// \details Contains all locations where the type of the requested symbol
    /// is defined. Each location specifies the exact file and character
    /// range where the type definition appears.
    struct lsp_locations_result result;
};

/// \brief Error response for failed type definition request.
/// \details Sent when the server cannot process the type definition request,
/// typically due to parsing errors, symbol resolution failures, or
/// unsupported file types.
///
/// \param error JSON-RPC error with specific failure details
/// \result None - operation failed
///
/// \since 3.6.0
/// \sa lsp_text_document_type_definition_request, lsp_error_result
/// \ingroup lsp_advanced
struct lsp_text_document_type_definition_error_result {
    /// \brief Common message header for error correlation.
    struct lsp_msg_hdr hdr;

    /// \brief Detailed error information.
    /// \details Contains the specific error code and message explaining
    /// why the type definition request could not be fulfilled.
    struct lsp_error_result error;
};
static struct lsp_text_document_type_definition_request*      lsp_init_text_document_type_definition_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_type_definition_response*     lsp_init_text_document_type_definition_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_type_definition_error_result* lsp_init_text_document_type_definition_error_result(void *mem, AkU64 tag_id) noexcept;

/// The `textDocument/documentHighlight` request is sent from the client to the server to resolve a document highlights for a given text document position.
/// The request's parameter is of type \ref lsp_text_document_position_params and the response is of type \ref lsp_document_highlight or a Thenable that resolves to such.
/// \since 3.0.0
struct lsp_text_document_document_highlight_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Text document position for highlights.
    struct lsp_text_document_position_params params;
};
/// \ingroup lsp_document_highlight_types
/// \brief Document highlight list
///
/// Contains all highlights for a symbol in a document.
struct lsp_document_highlight_list {
    struct lsp_list<struct lsp_document_highlight>* head;  ///< List of highlights
    int count;                                             ///< Number of highlights
};

/// \brief Final document highlights response.
struct lsp_text_document_document_highlight_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Array of document highlights.
    struct lsp_document_highlight_list result;
};
/// \brief Error result for document highlight request.
struct lsp_text_document_document_highlight_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_text_document_document_highlight_request*      lsp_init_text_document_document_highlight_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_document_highlight_response*     lsp_init_text_document_document_highlight_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_document_highlight_error_result* lsp_init_text_document_document_highlight_error_result(void *mem, AkU64 tag_id) noexcept;

/// \defgroup lsp_document_symbol_types Document Symbol Types
/// \brief Types for document symbol requests and responses
///
/// Types used specifically for document symbol operations.

/// \ingroup lsp_document_symbol_types
/// \brief Document symbol parameters
///
/// Parameters for a document symbol request.
/// Contains the text document identifier for which symbols should be listed.
struct lsp_document_symbol_params {
    struct lsp_text_document_identifier text_document;  ///< Document to analyze
};

/// The `textDocument/documentSymbol` request is sent from the client to the server to list all symbols found in a given text document.
/// The request's parameter is of type \ref lsp_document_symbol_params and the response is of type \ref lsp_symbol_information or \ref lsp_document_symbol or a Thenable that resolves to such.
/// \since 3.0.0
struct lsp_text_document_document_symbol_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Document symbol parameters.
    struct lsp_document_symbol_params params;
};
/// \brief Document symbol result discriminant
///
/// Defines the possible types of results for document symbol requests.
enum lsp_document_symbol_result_kind {
    LSP_DOC_SYMBOL_NONE = 0,       ///< No symbols found
    LSP_DOC_SYMBOL_HIERARCHICAL,   ///< Hierarchical symbol tree
    LSP_DOC_SYMBOL_FLAT            ///< Flat symbol list
};

/// \ingroup lsp_document_symbol_types
/// \brief Document symbol result union
///
/// Union type representing the result of a document symbol request.
/// Can contain either a hierarchical tree or a flat list of symbols.
struct lsp_document_symbol_result {
    int kind;  ///< lsp_document_symbol_result_kind discriminant
    union {
        struct lsp_list<struct lsp_document_symbol>* as_hierarchical;     ///< Hierarchical symbols
        struct lsp_list<struct lsp_symbol_information>* as_flat;          ///< Flat symbols
    } value;
};

/// \brief Final document symbols response.
struct lsp_text_document_document_symbol_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Document symbols (hierarchical or flat).
    struct lsp_document_symbol_result result;
};
/// \brief Error result for document symbol request.
struct lsp_text_document_document_symbol_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_text_document_document_symbol_request*      lsp_init_text_document_document_symbol_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_document_symbol_response*     lsp_init_text_document_document_symbol_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_document_symbol_error_result* lsp_init_text_document_document_symbol_error_result(void *mem, AkU64 tag_id) noexcept;


/// \defgroup lsp_completion_types Completion Types
/// \brief Types for completion requests and responses
///
/// Types used specifically for code completion functionality.

/// \ingroup lsp_completion_types
/// \brief Completion item kind enumeration
///
/// Defines the different kinds of completion items that can be provided.
/// Used to categorize completion suggestions for better user experience.

/// \ingroup lsp_completion_types
/// \brief Completion item tag enumeration (see earlier definition)

/// \ingroup lsp_completion_types
/// \brief Completion item tags list
///

/// \ingroup lsp_completion_types
/// \brief Text edit for completion item
///
/// Represents a text edit operation for a completion item.
/// Can be either a regular text edit or an insert/replace edit.

/// \ingroup lsp_completion_types
/// \brief Insert text format enumeration
///
/// Defines how the insert text for a completion item should be interpreted.
/// Determines whether the text is plain text or a snippet with placeholders.



/// \ingroup lsp_completion_types
/// \brief Completion result kind enumeration
///
/// Defines the possible types of completion results.
/// The result can be null, a completion list, or a list of items.
enum lsp_completion_result_kind {
    LSP_COMPLETION_NONE = 0,   ///< No completion result
    LSP_COMPLETION_LIST,       ///< Completion list result
    LSP_COMPLETION_ITEMS       ///< Completion items list result
};


/// \defgroup lsp_code_action_types Code Action Types
/// \brief Types for code action requests and responses
///
/// Types used specifically for code action operations.

/// \ingroup lsp_code_action_types
/// \brief Code action context
///
/// Contains information about the context in which code actions are requested.
/// Includes diagnostics and filtering criteria for available actions.
struct lsp_code_action_context {
    struct lsp_list<struct lsp_diagnostic>* diagnostics;  ///< Related diagnostics
    struct lsp_list<lsp_string>* only;                   ///< Filter by action kinds
};

/// \ingroup lsp_code_action_types
/// \brief Code action parameters
///
/// Parameters for a code action request.
/// Specifies the document, range, and context for computing available code actions.
struct lsp_code_action_params {
    struct lsp_text_document_identifier text_document;  ///< Target document
    struct lsp_range range;                            ///< Target range
    struct lsp_code_action_context context;            ///< Request context
};

/// The `textDocument/codeAction` request is sent from the client to the server to compute commands for a given text document and range.
/// The request's parameter is of type \ref lsp_code_action_params and the response is of type \ref lsp_code_action or a Thenable that resolves to such.
/// These commands are typically code fixes to either fix problems or to beautify/refactor code.
/// \since 3.0.0
struct lsp_text_document_code_action_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Code action parameters including range and context.
    struct lsp_code_action_params params;
};
/// \brief Final code actions response.
struct lsp_text_document_code_action_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Array of code actions or null.
    struct lsp_code_action_result result;
};
/// \brief Error result for code action request.
struct lsp_text_document_code_action_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_text_document_code_action_request*      lsp_init_text_document_code_action_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_code_action_response*     lsp_init_text_document_code_action_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_code_action_error_result* lsp_init_text_document_code_action_error_result(void *mem, AkU64 tag_id) noexcept;

/// The `codeAction/resolve` request is sent from the client to the server to resolve additional information for a given code action.
/// The request's parameter is of type \ref lsp_code_action and the response is of type \ref lsp_code_action or a Thenable that resolves to such.
/// This request is sent when a code action is selected in the user interface and the client needs to resolve additional information about the action.
/// \ingroup lsp_code_action_types
/// \brief Code action structure
///
/// Represents a code action that can be performed to fix or improve code.
struct lsp_code_action {
    lsp_string title;                           ///< Action title
    lsp_opt_string kind;                        ///< Optional action kind
    struct lsp_list<lsp_string>* diagnostics;   ///< Associated diagnostics
    lsp_opt_bool is_preferred;                  ///< Whether this is the preferred action
    lsp_opt_string disabled_present;            ///< Whether action is disabled
    struct lsp_code_action_disabled disabled;   ///< Disabled info (if present)
    lsp_opt_string edit_present;                ///< Whether workspace edit is present
    struct lsp_workspace_edit edit;             ///< Workspace edit (if present)
    lsp_opt_string command_present;             ///< Whether command is present
    struct lsp_dyn command;                     ///< Command (if present)
    lsp_opt_string data;                        ///< Optional action data
};

/// \since 3.16.0
struct lsp_code_action_resolve_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Code action to resolve.
    struct lsp_code_action params;
};
/// \brief Final resolved code action.
struct lsp_code_action_resolve_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Resolved code action with additional details.
    struct lsp_code_action result;
};
/// \brief Error result for code action resolve.
struct lsp_code_action_resolve_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_code_action_resolve_request*      lsp_init_code_action_resolve_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_code_action_resolve_response*     lsp_init_code_action_resolve_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_code_action_resolve_error_result* lsp_init_code_action_resolve_error_result(void *mem, AkU64 tag_id) noexcept;

/// \defgroup lsp_code_lens_types Code Lens Types
/// \brief Types for code lens requests and responses
///
/// Types used specifically for code lens operations.

/// \ingroup lsp_code_lens_types
/// \brief Code lens parameters
///
/// Parameters for a code lens request.
/// Contains the text document identifier for which code lenses should be computed.
struct lsp_code_lens_params {
    struct lsp_text_document_identifier text_document;  ///< Document to analyze
};

/// The `textDocument/codeLens` request is sent from the client to the server to compute code lenses for a given text document.
/// The request's parameter is of type \ref lsp_code_lens_params and the response is of type \ref lsp_code_lens or a Thenable that resolves to such.
/// \since 3.0.0
struct lsp_text_document_code_lens_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Code lens parameters.
    struct lsp_code_lens_params params;
};
/// \ingroup lsp_code_lens_types
/// \brief Code lens structure
///
/// Represents a code lens that provides additional information or actions
/// for a specific range in the document.
struct lsp_code_lens {
    lsp_range range;                     ///< Range where lens appears
    lsp_opt_string command_present;      ///< Whether command is present
    struct lsp_dyn command;              ///< Command to execute (if present)
    lsp_opt_string data;                 ///< Optional lens data
};

/// \ingroup lsp_code_lens_types
/// \brief Code lens list
///
/// Contains all code lenses for a document.
struct lsp_code_lens_list {
    struct lsp_list<struct lsp_code_lens>* head;  ///< List of code lenses
    int count;                                    ///< Number of code lenses
};

/// \brief Final code lenses response.
struct lsp_text_document_code_lens_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Array of code lenses.
    struct lsp_code_lens_list result;
};
/// \brief Error result for code lens request.
struct lsp_text_document_code_lens_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_text_document_code_lens_request*      lsp_init_text_document_code_lens_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_code_lens_response*     lsp_init_text_document_code_lens_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_code_lens_error_result* lsp_init_text_document_code_lens_error_result(void *mem, AkU64 tag_id) noexcept;

/// The `codeLens/resolve` request is sent from the client to the server to resolve a command for a given code lens.
/// The request's parameter is of type {@link CodeLens} and the response is of type {@link CodeLens} or a Thenable that resolves to such.
/// \since 3.0.0
struct lsp_code_lens_resolve_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Code lens to resolve.
    struct lsp_code_lens params;
};
/// \brief Final resolved code lens.
struct lsp_code_lens_resolve_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Resolved code lens with command.
    struct lsp_code_lens result;
};
/// \brief Error result for code lens resolve.
struct lsp_code_lens_resolve_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_code_lens_resolve_request*      lsp_init_code_lens_resolve_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_code_lens_resolve_response*     lsp_init_code_lens_resolve_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_code_lens_resolve_error_result* lsp_init_code_lens_resolve_error_result(void *mem, AkU64 tag_id) noexcept;

/// The `workspace/codeLens/refresh` request is sent from the server to the client to refresh all code lenses.
/// This request has no parameters and the response has no result.
/// \since 3.16.0
struct lsp_workspace_code_lens_refresh_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
};
/// \brief Response to code lens refresh (null result).
struct lsp_workspace_code_lens_refresh_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
};
/// \brief Error result for code lens refresh.
struct lsp_workspace_code_lens_refresh_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_workspace_code_lens_refresh_request*      lsp_init_workspace_code_lens_refresh_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_workspace_code_lens_refresh_response*     lsp_init_workspace_code_lens_refresh_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_workspace_code_lens_refresh_error_result* lsp_init_workspace_code_lens_refresh_error_result(void *mem, AkU64 tag_id) noexcept;

/// \defgroup lsp_document_link_types Document Link Types
/// \brief Types for document link requests and responses
///
/// Types used specifically for document link operations.

/// \ingroup lsp_document_link_types
/// \brief Document link parameters
///
/// Parameters for a document link request.
/// Contains the text document identifier for which document links should be computed.
struct lsp_document_link_params {
    struct lsp_text_document_identifier text_document;  ///< Document to analyze
};

/// The `textDocument/documentLink` request is sent from the client to the server to request the document links for a given text document.
/// The request's parameter is of type \ref lsp_document_link_params and the response is of type \ref lsp_document_link or a Thenable that resolves to such.
/// \since 3.0.0
struct lsp_text_document_document_link_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Document link parameters.
    struct lsp_document_link_params params;
};
/// \ingroup lsp_document_link_types
/// \brief Document link structure
///
/// Represents a link from one location in a document to another location,
/// potentially in a different document or resource.
struct lsp_document_link {
    lsp_range range;              ///< Range of the link
    lsp_opt_string target;        ///< Optional target URI
    lsp_opt_string tooltip;       ///< Optional tooltip text
    lsp_opt_string data;          ///< Optional link data
};

/// \ingroup lsp_document_link_types
/// \brief Document link list
///
/// Contains all document links for a document.
struct lsp_document_link_list {
    struct lsp_list<struct lsp_document_link>* head;  ///< List of document links
    int count;                                        ///< Number of links
};

/// \brief Final document links response.
struct lsp_text_document_document_link_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Array of document links.
    struct lsp_document_link_list result;
};
/// \brief Error result for document link request.
struct lsp_text_document_document_link_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_text_document_document_link_request*      lsp_init_text_document_document_link_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_document_link_response*     lsp_init_text_document_document_link_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_document_link_error_result* lsp_init_text_document_document_link_error_result(void *mem, AkU64 tag_id) noexcept;

/// The `documentLink/resolve` request is sent from the client to the server to resolve the target of a given document link.
/// The request's parameter is of type {@link DocumentLink} and the response is of type {@link DocumentLink} or a Thenable that resolves to such.
/// \since 3.0.0
struct lsp_document_link_resolve_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Document link to resolve.
    struct lsp_document_link params;
};
/// \brief Final resolved document link.
struct lsp_document_link_resolve_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Resolved document link with target.
    struct lsp_document_link result;
};
/// \brief Error result for document link resolve.
struct lsp_document_link_resolve_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_document_link_resolve_request*      lsp_init_document_link_resolve_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_document_link_resolve_response*     lsp_init_document_link_resolve_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_document_link_resolve_error_result* lsp_init_document_link_resolve_error_result(void *mem, AkU64 tag_id) noexcept;

/// \ingroup lsp_formatting_types
/// \brief Document formatting parameters
///
/// Parameters for formatting an entire document.
struct lsp_document_formatting_params {
    struct lsp_text_document_identifier text_document;  ///< Document to format
    struct lsp_dyn options;                             ///< Formatting options
};

/// \ingroup lsp_formatting_types
/// \brief Document range formatting parameters
///
/// Parameters for formatting a specific range within a document.
struct lsp_document_range_formatting_params {
    struct lsp_text_document_identifier text_document;  ///< Document containing range
    lsp_range range;                                    ///< Range to format
    struct lsp_dyn options;                             ///< Formatting options
};

/// \ingroup lsp_formatting_types
/// \brief Document ranges formatting parameters
///
/// Parameters for formatting multiple ranges within a document.
struct lsp_document_ranges_formatting_params {
    struct lsp_text_document_identifier text_document;  ///< Document containing ranges
    struct lsp_list<lsp_range>* ranges;                ///< Ranges to format
    struct lsp_dyn options;                             ///< Formatting options
};

/// \ingroup lsp_formatting_types
/// \brief Document on-type formatting parameters
///
/// Parameters for formatting as the user types.
struct lsp_document_on_type_formatting_params {
    struct lsp_text_document_identifier text_document;  ///< Document being edited
    lsp_position position;                              ///< Current cursor position
    lsp_string ch;                                      ///< Character that triggered formatting
    struct lsp_dyn options;                             ///< Formatting options
};

/// The `textDocument/formatting` request is sent from the client to the server to format a whole document.
/// The request's parameter is of type {@link DocumentFormattingParams} and the response is of type {@link TextEdit TextEdit[]} or a Thenable that resolves to such.
/// \since 3.0.0
struct lsp_text_document_formatting_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Document formatting parameters.
    struct lsp_document_formatting_params params;
};
/// \brief Final formatting response.
struct lsp_text_document_formatting_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Array of text edits or null.
    struct lsp_text_edit_list result;
};
/// \brief Error result for formatting request.
struct lsp_text_document_formatting_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_text_document_formatting_request*      lsp_init_text_document_formatting_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_formatting_response*     lsp_init_text_document_formatting_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_formatting_error_result* lsp_init_text_document_formatting_error_result(void *mem, AkU64 tag_id) noexcept;

/// The `textDocument/rangeFormatting` request is sent from the client to the server to format a given range in a document.
/// The request's parameter is of type {@link DocumentRangeFormattingParams} and the response is of type {@link TextEdit TextEdit[]} or a Thenable that resolves to such.
/// \since 3.0.0
struct lsp_text_document_range_formatting_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Range formatting parameters.
    struct lsp_document_range_formatting_params params;
};
/// \brief Final range formatting response.
struct lsp_text_document_range_formatting_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Array of text edits or null.
    struct lsp_text_edit_list result;
};
/// \brief Error result for range formatting request.
struct lsp_text_document_range_formatting_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_text_document_range_formatting_request*      lsp_init_text_document_range_formatting_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_range_formatting_response*     lsp_init_text_document_range_formatting_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_range_formatting_error_result* lsp_init_text_document_range_formatting_error_result(void *mem, AkU64 tag_id) noexcept;

/// The `textDocument/rangesFormatting` request is sent from the client to the server to format multiple ranges in a document.
/// The request's parameter is of type {@link DocumentRangesFormattingParams} and the response is of type {@link TextEdit TextEdit[]} or a Thenable that resolves to such.
/// \since 3.18.0
struct lsp_text_document_ranges_formatting_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Ranges formatting parameters.
    struct lsp_document_ranges_formatting_params params;
};
/// \brief Final ranges formatting response.
struct lsp_text_document_ranges_formatting_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Array of text edits or null.
    struct lsp_text_edit_list result;
};
/// \brief Error result for ranges formatting request.
struct lsp_text_document_ranges_formatting_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_text_document_ranges_formatting_request*      lsp_init_text_document_ranges_formatting_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_ranges_formatting_response*     lsp_init_text_document_ranges_formatting_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_ranges_formatting_error_result* lsp_init_text_document_ranges_formatting_error_result(void *mem, AkU64 tag_id) noexcept;

/// The `textDocument/onTypeFormatting` request is sent from the client to the server to format a document on type.
/// The request's parameter is of type {@link DocumentOnTypeFormattingParams} and the response is of type {@link TextEdit TextEdit[]} or a Thenable that resolves to such.
/// \since 3.0.0
struct lsp_text_document_on_type_formatting_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief On-type formatting parameters.
    struct lsp_document_on_type_formatting_params params;
};
/// \brief Final on-type formatting response.
struct lsp_text_document_on_type_formatting_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Array of text edits or null.
    struct lsp_text_edit_list result;
};
/// \brief Error result for on-type formatting request.
struct lsp_text_document_on_type_formatting_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_text_document_on_type_formatting_request*      lsp_init_text_document_on_type_formatting_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_on_type_formatting_response*     lsp_init_text_document_on_type_formatting_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_on_type_formatting_error_result* lsp_init_text_document_on_type_formatting_error_result(void *mem, AkU64 tag_id) noexcept;


/// \ingroup lsp_document_types
/// \brief Rename parameters
///
/// Parameters for a rename request, containing the position to rename
/// and the new name to use.
struct lsp_rename_params {
    struct lsp_text_document_position_params text_document_position;  ///< Position to rename
    lsp_string new_name;                                              ///< New name for the symbol
};

/// The `textDocument/rename` request is sent from the client to the server to perform a workspace-wide rename of a symbol.
/// The request's parameter is of type {@link RenameParams} and the response is of type {@link WorkspaceEdit} or a Thenable that resolves to such.
/// \since 3.0.0
struct lsp_text_document_rename_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Rename parameters including position and new name.
    // forward-declared below; ensure full definition appears before usage
    struct lsp_rename_params params;
};

/// \brief Final rename response.
struct lsp_text_document_rename_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Workspace edit with changes or null.
    struct lsp_workspace_edit result;
};


/// \brief Error result for rename request.
struct lsp_text_document_rename_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_text_document_rename_request*      lsp_init_text_document_rename_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_rename_response*     lsp_init_text_document_rename_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_rename_error_result* lsp_init_text_document_rename_error_result(void *mem, AkU64 tag_id) noexcept;

/// The `textDocument/prepareRename` request is sent from the client to the server to test and collect information about a rename operation.
/// The request's parameter is of type {@link TextDocumentPosition} and the response is of type {@link PrepareRenameResult} or a Thenable that resolves to such.
/// \since 3.0.0
struct lsp_text_document_prepare_rename_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Text document position for prepare rename.
    struct lsp_text_document_position_params params;
};

/// \ingroup lsp_document_types
/// \brief Prepare rename result union
///
/// Union type representing the result of a prepare rename request.
/// Can contain a range or a range with placeholder text.
struct lsp_prepare_rename_result {
    int kind;  ///< lsp_prepare_rename_result_kind discriminant
    union {
        lsp_range range;                           ///< Range (if kind == LSP_PREP_RENAME_RANGE)
        struct {
            lsp_range range;                       ///< Range
            lsp_string placeholder;                ///< Placeholder text
        } range_with_placeholder;                   ///< Range with placeholder (if kind == LSP_PREP_RENAME_RANGE_WITH_PLACEHOLDER)
    } value;
};

/// \brief Final prepare rename response.
struct lsp_text_document_prepare_rename_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Prepare rename result or null.
    // forward-declared below; ensure full definition appears before usage
    struct lsp_prepare_rename_result result;
};

/// \brief Error result for prepare rename request.
struct lsp_text_document_prepare_rename_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_text_document_prepare_rename_request*      lsp_init_text_document_prepare_rename_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_prepare_rename_response*     lsp_init_text_document_prepare_rename_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_prepare_rename_error_result* lsp_init_text_document_prepare_rename_error_result(void *mem, AkU64 tag_id) noexcept;

/// \defgroup lsp_semantic_tokens_types Semantic Tokens Types
/// \brief Types for semantic tokens requests and responses
///
/// Types used specifically for semantic tokens operations.

/// \ingroup lsp_semantic_tokens_types
/// \brief Semantic tokens edit
///
/// Represents a single edit operation on semantic tokens.
/// Used for incremental updates to semantic token information.
struct lsp_semantic_tokens_edit {
    lsp_opt_uinteger start;                          ///< Start position of the edit
    lsp_opt_uinteger delete_count;                   ///< Number of tokens to delete
    struct lsp_list<unsigned int>* data;             ///< New token data (optional)
};

/// \ingroup lsp_semantic_tokens_types
/// \brief Semantic tokens edits
///
/// Contains a list of edits to apply to semantic tokens.
/// Used for delta updates to semantic token information.
struct lsp_semantic_tokens_edits {
    lsp_opt_string result_id;                                    ///< Optional result identifier
    struct lsp_list<struct lsp_semantic_tokens_edit>* edits;     ///< List of edits to apply
};

/// \ingroup lsp_semantic_tokens_types
/// \brief Semantic tokens delta parameters
///
/// Parameters for a semantic tokens delta request.
/// Contains the document and the previous result ID for incremental updates.
struct lsp_semantic_tokens_delta_params {
    struct lsp_text_document_identifier text_document;  ///< Document to analyze
    lsp_string previous_result_id;                      ///< Previous result identifier
};

/// \ingroup lsp_semantic_tokens_types
/// \brief Semantic tokens range parameters
///
/// Parameters for a semantic tokens range request.
/// Contains the document and range for which semantic tokens should be computed.
struct lsp_semantic_tokens_range_params {
    struct lsp_text_document_identifier text_document;  ///< Document containing the range
    lsp_range range;                                    ///< Range to analyze
};

/// \ingroup lsp_semantic_tokens_types
/// \brief Semantic tokens structure
///
/// Contains the semantic token data for a document or range.
/// The data is encoded as an array of unsigned integers.
struct lsp_semantic_tokens {
    lsp_opt_string result_id;                    ///< Optional result identifier
    struct lsp_list<unsigned int>* data;         ///< Encoded token data
};

/// \ingroup lsp_semantic_tokens_types
/// \brief Semantic tokens delta result kind enumeration
///
/// Defines the possible types of results for semantic tokens delta requests.
/// The result can be either new tokens or edits to apply.
enum lsp_semantic_tokens_delta_result_kind {
    LSP_SEMANTIC_TOKENS_DELTA_TOKENS = 0,  ///< New semantic tokens
    LSP_SEMANTIC_TOKENS_DELTA_EDITS        ///< Edits to apply
};

/// \ingroup lsp_semantic_tokens_types
/// \brief Semantic tokens delta result union
///
/// Union type representing the result of a semantic tokens delta request.
/// Can contain either new tokens or edits to apply to existing tokens.
struct lsp_semantic_tokens_delta_result {
    int kind;  ///< lsp_semantic_tokens_delta_result_kind discriminant
    union {
        struct lsp_semantic_tokens tokens;           ///< New tokens (if kind == LSP_SEMANTIC_TOKENS_DELTA_TOKENS)
        struct lsp_semantic_tokens_edits edits;      ///< Edits to apply (if kind == LSP_SEMANTIC_TOKENS_DELTA_EDITS)
    } value;
};

/// \ingroup lsp_semantic_tokens_types
/// \brief Semantic tokens parameters
///
/// Parameters for a semantic tokens request.
/// Contains the text document identifier for which semantic tokens should be computed.
struct lsp_semantic_tokens_params {
    struct lsp_text_document_identifier text_document;  ///< Document to analyze
};

/// The `textDocument/semanticTokens/full` request is sent from the client to the server to return the semantic tokens for the given whole document.
/// The request's parameter is of type \ref lsp_semantic_tokens_params and the response is of type \ref lsp_semantic_tokens or a Thenable that resolves to such.
/// \since 3.16.0
struct lsp_text_document_semantic_tokens_full_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Semantic tokens parameters.
    struct lsp_semantic_tokens_params params;
};
/// \brief Final semantic tokens response.
struct lsp_text_document_semantic_tokens_full_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Semantic tokens for the document.
    struct lsp_semantic_tokens result;
};
/// \brief Error result for semantic tokens full request.
struct lsp_text_document_semantic_tokens_full_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_text_document_semantic_tokens_full_request*      lsp_init_text_document_semantic_tokens_full_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_semantic_tokens_full_response*     lsp_init_text_document_semantic_tokens_full_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_semantic_tokens_full_error_result* lsp_init_text_document_semantic_tokens_full_error_result(void *mem, AkU64 tag_id) noexcept;

/// The `textDocument/semanticTokens/full/delta` request is sent from the client to the server to return the semantic tokens delta for the given document.
/// The request's parameter is of type {@link SemanticTokensDeltaParams} and the response is of type {@link SemanticTokens} or {@link SemanticTokensDelta} or a Thenable that resolves to such.
/// \since 3.16.0
struct lsp_text_document_semantic_tokens_full_delta_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Semantic tokens delta parameters.
    struct lsp_semantic_tokens_delta_params params;
};
/// \brief Final semantic tokens delta response.
struct lsp_text_document_semantic_tokens_full_delta_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Semantic tokens delta result.
    struct lsp_semantic_tokens_delta_result result;
};
/// \brief Error result for semantic tokens delta request.
struct lsp_text_document_semantic_tokens_full_delta_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_text_document_semantic_tokens_full_delta_request*      lsp_init_text_document_semantic_tokens_full_delta_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_semantic_tokens_full_delta_response*     lsp_init_text_document_semantic_tokens_full_delta_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_semantic_tokens_full_delta_error_result* lsp_init_text_document_semantic_tokens_full_delta_error_result(void *mem, AkU64 tag_id) noexcept;

/// The `textDocument/semanticTokens/range` request is sent from the client to the server to return the semantic tokens for the given range.
/// The request's parameter is of type {@link SemanticTokensRangeParams} and the response is of type {@link SemanticTokens} or a Thenable that resolves to such.
/// \since 3.16.0
struct lsp_text_document_semantic_tokens_range_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Semantic tokens range parameters.
    struct lsp_semantic_tokens_range_params params;
};
/// \brief Final semantic tokens range response.
struct lsp_text_document_semantic_tokens_range_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Semantic tokens for the range.
    struct lsp_semantic_tokens result;
};
/// \brief Error result for semantic tokens range request.
struct lsp_text_document_semantic_tokens_range_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_text_document_semantic_tokens_range_request*      lsp_init_text_document_semantic_tokens_range_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_semantic_tokens_range_response*     lsp_init_text_document_semantic_tokens_range_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_semantic_tokens_range_error_result* lsp_init_text_document_semantic_tokens_range_error_result(void *mem, AkU64 tag_id) noexcept;

/// The `workspace/semanticTokens/refresh` request is sent from the server to the client to refresh all semantic tokens.
/// This request has no parameters and the response has no result.
/// \since 3.16.0
struct lsp_workspace_semantic_tokens_refresh_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
};
/// \brief Response to semantic tokens refresh (null result).
struct lsp_workspace_semantic_tokens_refresh_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
};
/// \brief Error result for semantic tokens refresh.
struct lsp_workspace_semantic_tokens_refresh_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_workspace_semantic_tokens_refresh_request*      lsp_init_workspace_semantic_tokens_refresh_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_workspace_semantic_tokens_refresh_response*     lsp_init_workspace_semantic_tokens_refresh_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_workspace_semantic_tokens_refresh_error_result* lsp_init_workspace_semantic_tokens_refresh_error_result(void *mem, AkU64 tag_id) noexcept;


/// \ingroup lsp_call_hierarchy_types
/// \brief Call hierarchy item list
///
/// Contains a list of call hierarchy items.
/// Used as the result of prepare call hierarchy requests.
struct lsp_call_hierarchy_item_list {
    struct lsp_list<struct lsp_call_hierarchy_item>* head;  ///< List of call hierarchy items
    int count;                                              ///< Number of items
};

/// The `textDocument/prepareCallHierarchy` request is sent from the client to the server to prepare for call hierarchy computation.
/// The request's parameter is of type {@link TextDocumentPosition} and the response is of type {@link CallHierarchyItem CallHierarchyItem[]} or a Thenable that resolves to such.
/// \since 3.16.0
struct lsp_text_document_prepare_call_hierarchy_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Text document position for call hierarchy.
    struct lsp_text_document_position_params params;
};
/// \brief Final prepare call hierarchy response.
struct lsp_text_document_prepare_call_hierarchy_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Call hierarchy items or null.
    // forward-declared below; ensure full definition appears before usage
    struct lsp_call_hierarchy_item_list result;
};
/// \brief Error result for prepare call hierarchy request.
struct lsp_text_document_prepare_call_hierarchy_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_text_document_prepare_call_hierarchy_request*      lsp_init_text_document_prepare_call_hierarchy_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_prepare_call_hierarchy_response*     lsp_init_text_document_prepare_call_hierarchy_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_prepare_call_hierarchy_error_result* lsp_init_text_document_prepare_call_hierarchy_error_result(void *mem, AkU64 tag_id) noexcept;

/// \defgroup lsp_call_hierarchy_types Call Hierarchy Types
/// \brief Types for call hierarchy requests and responses
///
/// Types used specifically for call hierarchy operations.

/// \ingroup lsp_call_hierarchy_types
/// \brief Call hierarchy item
///
/// Represents an item in the call hierarchy, such as a function or method.
/// Contains information about the item's name, kind, location, and other properties.
struct lsp_call_hierarchy_item {
    lsp_string name;                      ///< Item name
    enum lsp_symbol_kind kind;            ///< Symbol kind
    struct lsp_symbol_tags tags;          ///< Symbol tags
    lsp_opt_string detail;               ///< Optional detail text
    lsp_uri uri;                          ///< Item URI
    lsp_range range;                      ///< Item range
    lsp_range selection_range;            ///< Selection range
    lsp_opt_string data;                  ///< Optional data
};



/// \ingroup lsp_call_hierarchy_types
/// \brief Call hierarchy incoming calls parameters
///
/// Parameters for incoming calls requests.
/// Contains the call hierarchy item for which to find incoming calls.
struct lsp_call_hierarchy_incoming_calls_params {
    struct lsp_call_hierarchy_item item;  ///< Item to find incoming calls for
};

/// \ingroup lsp_call_hierarchy_types
/// \brief Call hierarchy outgoing calls parameters
///
/// Parameters for outgoing calls requests.
/// Contains the call hierarchy item for which to find outgoing calls.
struct lsp_call_hierarchy_outgoing_calls_params {
    struct lsp_call_hierarchy_item item;  ///< Item to find outgoing calls for
};

/// \ingroup lsp_call_hierarchy_types
/// \brief Call hierarchy incoming call
///
/// Represents an incoming call to a call hierarchy item.
/// Contains the caller item and the ranges where the call occurs.
struct lsp_call_hierarchy_incoming_call {
    struct lsp_call_hierarchy_item from;       ///< Caller item
    struct lsp_list<lsp_range>* from_ranges;   ///< Ranges where the call occurs
};

/// \ingroup lsp_call_hierarchy_types
/// \brief Call hierarchy outgoing call
///
/// Represents an outgoing call from a call hierarchy item.
/// Contains the callee item and the ranges where the call occurs.
struct lsp_call_hierarchy_outgoing_call {
    struct lsp_call_hierarchy_item to;         ///< Callee item
    struct lsp_list<lsp_range>* from_ranges;   ///< Ranges where the call occurs
};

/// \ingroup lsp_call_hierarchy_types
/// \brief Call hierarchy incoming call list
///
/// Contains a list of incoming calls for a call hierarchy item.
struct lsp_call_hierarchy_incoming_call_list {
    struct lsp_list<struct lsp_call_hierarchy_incoming_call>* head;  ///< List of incoming calls
    int count;                                                       ///< Number of incoming calls
};

/// \ingroup lsp_call_hierarchy_types
/// \brief Call hierarchy outgoing call list
///
/// Contains a list of outgoing calls from a call hierarchy item.
struct lsp_call_hierarchy_outgoing_call_list {
    struct lsp_list<struct lsp_call_hierarchy_outgoing_call>* head;  ///< List of outgoing calls
    int count;                                                       ///< Number of outgoing calls
};

/// The `callHierarchy/incomingCalls` request is sent from the client to the server to resolve incoming calls for a given call hierarchy item.
/// The request's parameter is of type {@link CallHierarchyIncomingCallsParams} and the response is of type {@link CallHierarchyIncomingCall CallHierarchyIncomingCall[]} or a Thenable that resolves to such.
/// \since 3.16.0
struct lsp_call_hierarchy_incoming_calls_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Call hierarchy incoming calls parameters.
    struct lsp_call_hierarchy_incoming_calls_params params;
};
/// \brief Final incoming calls response.
struct lsp_call_hierarchy_incoming_calls_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Array of incoming calls.
    struct lsp_call_hierarchy_incoming_call_list result;
};
/// \brief Error result for incoming calls request.
struct lsp_call_hierarchy_incoming_calls_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_call_hierarchy_incoming_calls_request*      lsp_init_call_hierarchy_incoming_calls_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_call_hierarchy_incoming_calls_response*     lsp_init_call_hierarchy_incoming_calls_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_call_hierarchy_incoming_calls_error_result* lsp_init_call_hierarchy_incoming_calls_error_result(void *mem, AkU64 tag_id) noexcept;

/// The `callHierarchy/outgoingCalls` request is sent from the client to the server to resolve outgoing calls for a given call hierarchy item.
/// The request's parameter is of type {@link CallHierarchyOutgoingCallsParams} and the response is of type {@link CallHierarchyOutgoingCall CallHierarchyOutgoingCall[]} or a Thenable that resolves to such.
/// \since 3.16.0
struct lsp_call_hierarchy_outgoing_calls_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Call hierarchy outgoing calls parameters.
    struct lsp_call_hierarchy_outgoing_calls_params params;
};
/// \brief Final outgoing calls response.
struct lsp_call_hierarchy_outgoing_calls_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Array of outgoing calls.
    struct lsp_call_hierarchy_outgoing_call_list result;
};
/// \brief Error result for outgoing calls request.
struct lsp_call_hierarchy_outgoing_calls_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_call_hierarchy_outgoing_calls_request*      lsp_init_call_hierarchy_outgoing_calls_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_call_hierarchy_outgoing_calls_response*     lsp_init_call_hierarchy_outgoing_calls_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_call_hierarchy_outgoing_calls_error_result* lsp_init_call_hierarchy_outgoing_calls_error_result(void *mem, AkU64 tag_id) noexcept;

/// \ingroup lsp_color_types
/// \brief Color information list
///
/// Contains all colors found in a document.
struct lsp_color_information_list {
    struct lsp_list<struct lsp_color_information>* head;  ///< List of color information
    int count;                                             ///< Number of color information entries
};

/// \ingroup lsp_color_types
/// \brief Document color parameters
///
/// Parameters for a document color request.
struct lsp_document_color_params {
    struct lsp_text_document_identifier text_document;  ///< Document to analyze
};

/// The `textDocument/documentColor` request is sent from the client to the server to list all color references in a given text document.
/// The request's parameter is of type {@link DocumentColorParams} and the response is of type {@link ColorInformation ColorInformation[]} or a Thenable that resolves to such.
/// \since 3.6.0
struct lsp_text_document_document_color_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Document color parameters.
    struct lsp_document_color_params params;
};

/// \brief Final document colors response.
struct lsp_text_document_document_color_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Array of color information.
    struct lsp_color_information_list result;
};

/// \brief Error result for document color request.
struct lsp_text_document_document_color_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_text_document_document_color_request*      lsp_init_text_document_document_color_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_document_color_response*     lsp_init_text_document_document_color_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_document_color_error_result* lsp_init_text_document_document_color_error_result(void *mem, AkU64 tag_id) noexcept;

/// \ingroup lsp_color_types
/// \brief Color presentation parameters
///
/// Parameters for a color presentation request, containing the
/// color to present and the range it applies to.
struct lsp_color_presentation_params {
    struct lsp_text_document_identifier text_document;  ///< Document containing the color
    struct lsp_color color;                             ///< Color to present
    struct lsp_range range;                             ///< Range where color appears
};

/// The `textDocument/colorPresentation` request is sent from the client to the server to request color presentations for a color.
/// The request's parameter is of type {@link ColorPresentationParams} and the response is of type {@link ColorPresentation ColorPresentation[]} or a Thenable that resolves to such.
/// \since 3.6.0
struct lsp_text_document_color_presentation_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Color presentation parameters.
    struct lsp_color_presentation_params params;
};

/// \ingroup lsp_color_types
/// \brief Color presentation list
///
/// Contains all possible presentations for a color.
struct lsp_color_presentation_list {
    struct lsp_list<struct lsp_color_presentation>* head;  ///< List of presentations
    int count;                                             ///< Number of presentations
};

/// \brief Final color presentations response.
struct lsp_text_document_color_presentation_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Array of color presentations.
    struct lsp_color_presentation_list result;
};

/// \brief Error result for color presentation request.
struct lsp_text_document_color_presentation_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};

static struct lsp_text_document_color_presentation_request*      lsp_init_text_document_color_presentation_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_color_presentation_response*     lsp_init_text_document_color_presentation_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_color_presentation_error_result* lsp_init_text_document_color_presentation_error_result(void *mem, AkU64 tag_id) noexcept;

/// \defgroup lsp_folding_range_types Folding Range Types
/// \brief Types for folding range requests and responses
///
/// Types used specifically for folding range operations.

/// \ingroup lsp_folding_range_types
/// \brief Folding range kind enumeration
///
/// Defines the different kinds of folding ranges that can be identified.
/// Used to categorize the type of code construct being folded.
enum lsp_folding_range_kind {
    LSP_FOLDING_COMMENT = 0,  ///< Comment folding range
    LSP_FOLDING_IMPORTS,      ///< Imports folding range
    LSP_FOLDING_REGION        ///< Region folding range
};

/// \ingroup lsp_folding_range_types
/// \brief Folding range structure
///
/// Represents a single folding range in a document.
/// Contains the start and end positions and optional metadata.
struct lsp_folding_range {
    unsigned int start_line;          ///< Starting line number (0-based)
    lsp_opt_uinteger start_character; ///< Optional starting character position
    unsigned int end_line;            ///< Ending line number (0-based)
    lsp_opt_uinteger end_character;   ///< Optional ending character position
    lsp_opt_string kind;              ///< Optional folding range kind
    lsp_opt_string collapsed_text;    ///< Optional text to display when collapsed
};

/// \ingroup lsp_folding_range_types
/// \brief Folding range list
///
/// Contains a list of folding ranges for a document.
/// Used as the result of folding range requests.
struct lsp_folding_range_list {
    int kind;  ///< LSP_OPT_NONE (null) or LSP_OPT_SOME (array)
    struct lsp_list<struct lsp_folding_range>* head;  ///< List of folding ranges (valid if kind == SOME)
    int count; ///< Number of folding ranges (valid if kind == SOME)
};

/// \ingroup lsp_folding_range_types
/// \brief Folding range parameters
///
/// Parameters for a folding range request.
/// Contains the text document identifier for which folding ranges should be computed.
struct lsp_folding_range_params {
    struct lsp_text_document_identifier text_document;  ///< Document to analyze
};

/// The `textDocument/foldingRange` request is sent from the client to the server to return all folding ranges found in a given text document.
/// The request's parameter is of type \ref lsp_folding_range_params and the response is of type \ref lsp_folding_range or a Thenable that resolves to such.
/// \since 3.10.0
struct lsp_text_document_folding_range_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Folding range parameters.
    struct lsp_folding_range_params params;
};
/// \brief Final folding ranges response.
struct lsp_text_document_folding_range_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Array of folding ranges or null.
    struct lsp_folding_range_list result;
};
/// \brief Error result for folding range request.
struct lsp_text_document_folding_range_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_text_document_folding_range_request*      lsp_init_text_document_folding_range_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_folding_range_response*     lsp_init_text_document_folding_range_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_folding_range_error_result* lsp_init_text_document_folding_range_error_result(void *mem, AkU64 tag_id) noexcept;

/// The `workspace/foldingRange/refresh` request is sent from the server to the client to refresh all folding ranges.
/// This request has no parameters and the response has no result.
/// \since 3.18.0
struct lsp_workspace_folding_range_refresh_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
};
/// \brief Response to folding range refresh (null result).
struct lsp_workspace_folding_range_refresh_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
};
/// \brief Error result for folding range refresh.
struct lsp_workspace_folding_range_refresh_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_workspace_folding_range_refresh_request*      lsp_init_workspace_folding_range_refresh_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_workspace_folding_range_refresh_response*     lsp_init_workspace_folding_range_refresh_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_workspace_folding_range_refresh_error_result* lsp_init_workspace_folding_range_refresh_error_result(void *mem, AkU64 tag_id) noexcept;

/// The `textDocument/linkedEditingRange` request is sent from the client to the server to return linked editing ranges for a position.
/// The request's parameter is of type {@link TextDocumentPosition} and the response is of type {@link LinkedEditingRanges} or a Thenable that resolves to such.
/// \since 3.16.0
struct lsp_text_document_linked_editing_range_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Text document position for linked editing.
    struct lsp_text_document_position_params params;
};
/// \brief Final linked editing ranges response.
struct lsp_text_document_linked_editing_range_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Linked editing ranges or null.
    struct lsp_linked_editing_ranges result;
};
/// \brief Error result for linked editing range request.
struct lsp_text_document_linked_editing_range_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_text_document_linked_editing_range_request*      lsp_init_text_document_linked_editing_range_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_linked_editing_range_response*     lsp_init_text_document_linked_editing_range_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_linked_editing_range_error_result* lsp_init_text_document_linked_editing_range_error_result(void *mem, AkU64 tag_id) noexcept;

/// The `textDocument/moniker` request is sent from the client to the server to resolve the monikers of a symbol at a given text document position.
/// The request's parameter is of type {@link TextDocumentPosition} and the response is of type {@link Moniker Moniker[]} or a Thenable that resolves to such.
/// \since 3.16.0
struct lsp_text_document_moniker_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Text document position for moniker.
    struct lsp_text_document_position_params params;
};
/// \brief Final moniker response.
struct lsp_text_document_moniker_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Array of monikers.
    struct lsp_moniker_list result;
};
/// \brief Error result for moniker request.
struct lsp_text_document_moniker_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_text_document_moniker_request*      lsp_init_text_document_moniker_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_moniker_response*     lsp_init_text_document_moniker_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_moniker_error_result* lsp_init_text_document_moniker_error_result(void *mem, AkU64 tag_id) noexcept;

/// Type hierarchy item
struct lsp_type_hierarchy_item {
    lsp_string name;
    enum lsp_symbol_kind kind;
    struct lsp_symbol_tags tags;
    lsp_opt_string detail;
    lsp_uri uri;
    lsp_range range;
    lsp_range selection_range;
    lsp_opt_string data;
};

/// Type hierarchy item list
struct lsp_type_hierarchy_item_list {
    struct lsp_list<struct lsp_type_hierarchy_item>* head;
    int count;
};

/// Type hierarchy supertypes parameters
struct lsp_type_hierarchy_supertypes_params {
    struct lsp_type_hierarchy_item item;
};

/// Type hierarchy subtypes parameters
struct lsp_type_hierarchy_subtypes_params {
    struct lsp_type_hierarchy_item item;
};

/// The `textDocument/prepareTypeHierarchy` request is sent from the client to the server to prepare for type hierarchy computation.
/// The request's parameter is of type {@link TextDocumentPosition} and the response is of type {@link TypeHierarchyItem TypeHierarchyItem[]} or a Thenable that resolves to such.
/// \since 3.17.0
struct lsp_text_document_prepare_type_hierarchy_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Text document position for type hierarchy.
    struct lsp_text_document_position_params params;
};
/// \brief Final prepare type hierarchy response.
struct lsp_text_document_prepare_type_hierarchy_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Type hierarchy items or null.
    struct lsp_type_hierarchy_item_list result;
};
/// \brief Error result for prepare type hierarchy request.
struct lsp_text_document_prepare_type_hierarchy_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_text_document_prepare_type_hierarchy_request*      lsp_init_text_document_prepare_type_hierarchy_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_prepare_type_hierarchy_response*     lsp_init_text_document_prepare_type_hierarchy_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_prepare_type_hierarchy_error_result* lsp_init_text_document_prepare_type_hierarchy_error_result(void *mem, AkU64 tag_id) noexcept;

/// The `typeHierarchy/supertypes` request is sent from the client to the server to resolve supertypes for a given type hierarchy item.
/// The request's parameter is of type {@link TypeHierarchySupertypesParams} and the response is of type {@link TypeHierarchyItem TypeHierarchyItem[]} or a Thenable that resolves to such.
/// \since 3.17.0
struct lsp_type_hierarchy_supertypes_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Type hierarchy supertypes parameters.
    struct lsp_type_hierarchy_supertypes_params params;
};
/// \brief Final supertypes response.
struct lsp_type_hierarchy_supertypes_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Array of supertype hierarchy items.
    struct lsp_type_hierarchy_item_list result;
};
/// \brief Error result for supertypes request.
struct lsp_type_hierarchy_supertypes_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_type_hierarchy_supertypes_request*      lsp_init_type_hierarchy_supertypes_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_type_hierarchy_supertypes_response*     lsp_init_type_hierarchy_supertypes_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_type_hierarchy_supertypes_error_result* lsp_init_type_hierarchy_supertypes_error_result(void *mem, AkU64 tag_id) noexcept;

/// The `typeHierarchy/subtypes` request is sent from the client to the server to resolve subtypes for a given type hierarchy item.
/// The request's parameter is of type {@link TypeHierarchySubtypesParams} and the response is of type {@link TypeHierarchyItem TypeHierarchyItem[]} or a Thenable that resolves to such.
/// \since 3.17.0
struct lsp_type_hierarchy_subtypes_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Type hierarchy subtypes parameters.
    struct lsp_type_hierarchy_subtypes_params params;
};
/// \brief Final subtypes response.
struct lsp_type_hierarchy_subtypes_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Array of subtype hierarchy items.
    struct lsp_type_hierarchy_item_list result;
};
/// \brief Error result for subtypes request.
struct lsp_type_hierarchy_subtypes_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_type_hierarchy_subtypes_request*      lsp_init_type_hierarchy_subtypes_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_type_hierarchy_subtypes_response*     lsp_init_type_hierarchy_subtypes_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_type_hierarchy_subtypes_error_result* lsp_init_type_hierarchy_subtypes_error_result(void *mem, AkU64 tag_id) noexcept;

/// \defgroup lsp_inline_value_types Inline Value Types
/// \brief Types for inline value requests and responses
///
/// Types used specifically for inline value operations.

/// \ingroup lsp_inline_value_types
/// \brief Inline value context
///
/// Context information for inline value evaluation.
/// Contains the frame ID and stopped location for debugging context.
struct lsp_inline_value_context {
    int frame_id;              ///< Stack frame identifier
    lsp_position stopped_location;  ///< Location where execution stopped
};

/// \ingroup lsp_inline_value_types
/// \brief Inline value parameters
///
/// Parameters for an inline value request.
/// Contains the text document identifier, range, and evaluation context.
struct lsp_inline_value_params {
    struct lsp_text_document_identifier text_document;  ///< Document to analyze
    lsp_range range;                                    ///< Range within document
    struct lsp_inline_value_context context;            ///< Evaluation context
};

/// The `textDocument/inlineValue` request is sent from the client to the server to compute inline values for a given text document.
/// The request's parameter is of type \ref lsp_inline_value_params and the response is of type \ref lsp_inline_value or a Thenable that resolves to such.
/// \since 3.17.0
struct lsp_text_document_inline_value_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Inline value parameters.
    struct lsp_inline_value_params params;
};
/// \brief Final inline values response.
struct lsp_text_document_inline_value_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Array of inline values.
    struct lsp_inline_value_list result;
};
/// \brief Error result for inline value request.
struct lsp_text_document_inline_value_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_text_document_inline_value_request*      lsp_init_text_document_inline_value_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_inline_value_response*     lsp_init_text_document_inline_value_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_inline_value_error_result* lsp_init_text_document_inline_value_error_result(void *mem, AkU64 tag_id) noexcept;

/// The `workspace/inlineValue/refresh` request is sent from the server to the client to refresh all inline values.
/// This request has no parameters and the response has no result.
/// \since 3.17.0
struct lsp_workspace_inline_value_refresh_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
};
/// \brief Response to inline value refresh (null result).
struct lsp_workspace_inline_value_refresh_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
};
/// \brief Error result for inline value refresh.
struct lsp_workspace_inline_value_refresh_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_workspace_inline_value_refresh_request*      lsp_init_workspace_inline_value_refresh_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_workspace_inline_value_refresh_response*     lsp_init_workspace_inline_value_refresh_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_workspace_inline_value_refresh_error_result* lsp_init_workspace_inline_value_refresh_error_result(void *mem, AkU64 tag_id) noexcept;

/// \defgroup lsp_inlay_hint_types Inlay Hint Types
/// \brief Types for inlay hint requests and responses
///
/// Types used specifically for inlay hint operations.

/// \ingroup lsp_inlay_hint_types
/// \brief Inlay hint parameters
///
/// Parameters for an inlay hint request.
/// Contains the text document identifier and range for which inlay hints should be computed.
struct lsp_inlay_hint_params {
    struct lsp_text_document_identifier text_document;  ///< Document to analyze
    lsp_range range;                                    ///< Range within document
};

/// The `textDocument/inlayHint` request is sent from the client to the server to request inlay hints for a given text document.
/// The request's parameter is of type \ref lsp_inlay_hint_params and the response is of type \ref lsp_inlay_hint or a Thenable that resolves to such.
/// \since 3.17.0
struct lsp_text_document_inlay_hint_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Inlay hint parameters.
    struct lsp_inlay_hint_params params;
};
/// \brief Final inlay hints response.
struct lsp_text_document_inlay_hint_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Array of inlay hints.
    struct lsp_inlay_hint_list result;
};
/// \brief Error result for inlay hint request.
struct lsp_text_document_inlay_hint_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_text_document_inlay_hint_request*      lsp_init_text_document_inlay_hint_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_inlay_hint_response*     lsp_init_text_document_inlay_hint_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_inlay_hint_error_result* lsp_init_text_document_inlay_hint_error_result(void *mem, AkU64 tag_id) noexcept;

/// The `inlayHint/resolve` request is sent from the client to the server to resolve additional information for a given inlay hint.
/// The request's parameter is of type {@link InlayHint} and the response is of type {@link InlayHint} or a Thenable that resolves to such.
/// \since 3.17.0
struct lsp_inlay_hint_resolve_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Inlay hint to resolve.
    struct lsp_inlay_hint params;
};
/// \brief Final resolved inlay hint.
struct lsp_inlay_hint_resolve_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Resolved inlay hint with additional details.
    struct lsp_inlay_hint result;
};
/// \brief Error result for inlay hint resolve.
struct lsp_inlay_hint_resolve_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_inlay_hint_resolve_request*      lsp_init_inlay_hint_resolve_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_inlay_hint_resolve_response*     lsp_init_inlay_hint_resolve_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_inlay_hint_resolve_error_result* lsp_init_inlay_hint_resolve_error_result(void *mem, AkU64 tag_id) noexcept;

/// The `workspace/inlayHint/refresh` request is sent from the server to the client to refresh all inlay hints.
/// This request has no parameters and the response has no result.
/// \since 3.17.0
struct lsp_workspace_inlay_hint_refresh_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
};
/// \brief Response to inlay hint refresh (null result).
struct lsp_workspace_inlay_hint_refresh_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
};
/// \brief Error result for inlay hint refresh.
struct lsp_workspace_inlay_hint_refresh_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_workspace_inlay_hint_refresh_request*      lsp_init_workspace_inlay_hint_refresh_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_workspace_inlay_hint_refresh_response*     lsp_init_workspace_inlay_hint_refresh_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_workspace_inlay_hint_refresh_error_result* lsp_init_workspace_inlay_hint_refresh_error_result(void *mem, AkU64 tag_id) noexcept;

/// \defgroup lsp_signature_help_types Signature Help Types
/// \brief Types for signature help requests and responses
///
/// Types used specifically for function signature assistance.

/// \ingroup lsp_signature_help_types
/// \brief Parameter information for function signatures
///
/// Represents a single parameter in a function signature with its
/// label and optional documentation.
struct lsp_signature_parameter {
    lsp_string label;              ///< Parameter label/name
    lsp_opt_string documentation;  ///< Optional parameter documentation
};

/// \ingroup lsp_signature_help_types
/// \brief Complete signature information
///
/// Represents a function signature with its label, documentation,
/// parameters, and active parameter information.
struct lsp_signature_information {
    lsp_string label;                                      ///< Signature label
    lsp_opt_string documentation;                          ///< Optional signature documentation
    struct lsp_list<struct lsp_signature_parameter>* parameters;  ///< Parameter list
    lsp_opt_uinteger active_parameter;                     ///< Currently active parameter index
};


/// \defgroup lsp_reference_types Reference Types
/// \brief Types for find references requests and responses
///
/// Types used specifically for finding symbol references.

/// \ingroup lsp_reference_types
/// \brief Reference search context
///
/// Controls what types of references to include in the search results.
/// Determines whether declarations should be included along with references.

/// \ingroup lsp_reference_types
/// \brief Find references parameters
///
/// Parameters for a find references request, combining position
/// information with reference search context.

/// \ingroup lsp_reference_types
/// \brief Locations result (array or null)
///
/// Represents either a list of locations or null, used for reference
/// search results and other location-based operations.

/// \defgroup lsp_document_highlight_types Document Highlight Types
/// \brief Types for document highlight requests and responses
///
/// Types used specifically for highlighting symbols in documents.

/// \ingroup lsp_document_highlight_types
/// \brief Document highlight kind enumeration
///
/// Specifies the type of highlighting to apply to a symbol occurrence.
/// Determines whether the highlight represents a read, write, or text reference.
enum lsp_document_highlight_kind {
    LSP_HIGHLIGHT_TEXT = 1,   ///< Text reference (general)
    LSP_HIGHLIGHT_READ = 2,   ///< Read access to symbol
    LSP_HIGHLIGHT_WRITE = 3   ///< Write access to symbol
};

/// \ingroup lsp_document_highlight_types
/// \brief Document highlight information
///
/// Represents a single highlighted range in a document with its
/// associated highlighting kind (read, write, or text).
struct lsp_document_highlight {
    lsp_range range;                           ///< Highlighted range
    enum lsp_document_highlight_kind kind;     ///< Type of highlight
};

/// \ingroup lsp_document_highlight_types
/// \brief Document highlight list
///
/// Contains all highlights for a symbol in a document.

/// \defgroup lsp_document_symbol_types Document Symbol Types
/// \brief Types for document symbol requests and responses
///
/// Types used specifically for getting symbols from documents.





/// \ingroup lsp_document_symbol_types
/// \brief Document symbol (hierarchical)
///
/// Represents a symbol in a document with hierarchical information.
/// Can contain child symbols, creating a tree structure.
struct lsp_document_symbol {
    lsp_string name;                                           ///< Symbol name
    lsp_opt_string detail;                                    ///< Optional symbol details
    enum lsp_symbol_kind kind;                                 ///< Symbol kind
    struct lsp_symbol_tags tags;                               ///< Symbol tags
    lsp_range range;                                           ///< Symbol range
    lsp_range selection_range;                                 ///< Symbol selection range
    lsp_opt_string children_present;                           ///< Whether children are present
    struct lsp_list<struct lsp_document_symbol>* children;     ///< Child symbols (if present)
};



/// \ingroup lsp_document_symbol_types

/// \defgroup lsp_code_action_types Code Action Types
/// \brief Types for code action requests and responses
///
/// Types used specifically for code action functionality.

/// \ingroup lsp_code_action_types
/// \brief Code action kind enumeration
///
/// Defines the different kinds of code actions that can be performed.
/// Used to categorize and filter available code actions.
enum lsp_code_action_kind {
    LSP_CODE_ACTION_EMPTY = 0,                      ///< Empty/placeholder action
    LSP_CODE_ACTION_QUICK_FIX,                      ///< Quick fix for a problem
    LSP_CODE_ACTION_REFACTOR,                       ///< General refactoring
    LSP_CODE_ACTION_REFACTOR_EXTRACT,               ///< Extract refactoring
    LSP_CODE_ACTION_REFACTOR_INLINE,                ///< Inline refactoring
    LSP_CODE_ACTION_REFACTOR_REWRITE,               ///< Rewrite refactoring
    LSP_CODE_ACTION_SOURCE,                         ///< Source-level action
    LSP_CODE_ACTION_SOURCE_ORGANIZE_IMPORTS,        ///< Organize imports
    LSP_CODE_ACTION_SOURCE_FIX_ALL                  ///< Fix all problems
};

/// \ingroup lsp_code_action_types
/// \brief Diagnostic severity enumeration
///
/// Defines the severity levels for diagnostics/problems.
/// Used to indicate the importance or urgency of issues.
enum lsp_diagnostic_severity {
    LSP_DIAGNOSTIC_ERROR = 1,       ///< Error severity
    LSP_DIAGNOSTIC_WARNING = 2,     ///< Warning severity
    LSP_DIAGNOSTIC_INFORMATION = 3, ///< Information severity
    LSP_DIAGNOSTIC_HINT = 4         ///< Hint severity
};

/// \ingroup lsp_code_action_types
/// \brief Diagnostic tag enumeration
///
/// Additional properties that can be attached to diagnostics.
enum lsp_diagnostic_tag {
    LSP_DIAGNOSTIC_UNNECESSARY = 1, ///< Diagnostic marks unnecessary code
    LSP_DIAGNOSTIC_DEPRECATED = 2   ///< Diagnostic marks deprecated usage
};

/// \ingroup lsp_code_action_types
/// \brief Diagnostic tags list
///
/// Contains a list of tags that apply to a diagnostic.
struct lsp_diagnostic_tags {
    struct lsp_list<enum lsp_diagnostic_tag>* head;  ///< List of diagnostic tags
    int count;                                       ///< Number of tags
};

/// \ingroup lsp_code_action_types
/// \brief Diagnostic structure
///
/// Represents a diagnostic (error, warning, etc.) found in source code.
struct lsp_diagnostic {
    lsp_range range;                           ///< Affected range
    enum lsp_diagnostic_severity severity;     ///< Diagnostic severity
    lsp_opt_string code;                       ///< Optional diagnostic code
    lsp_opt_string code_description;           ///< Optional code description
    lsp_opt_string source;                     ///< Diagnostic source
    lsp_string message;                        ///< Diagnostic message
    struct lsp_diagnostic_tags tags;           ///< Diagnostic tags
    lsp_opt_string related_information_present; ///< Whether related info is present
    struct lsp_list<struct lsp_diagnostic_related_information>* related_information; ///< Related info (if present)
};

/// \ingroup lsp_code_action_types
/// \brief Diagnostic related information
///
/// Contains additional location information related to a diagnostic.
struct lsp_diagnostic_related_information {
    struct lsp_location location;  ///< Related location
    lsp_string message;            ///< Related message
};


// /// \ingroup lsp_code_action_types
// /// \brief Code action result
// ///
// /// Union type representing the result of a code action request.
// struct lsp_code_action_result {
//     int kind;  ///< LSP_OPT_NONE or LSP_OPT_SOME
//     struct lsp_list<struct lsp_code_action>* value;  ///< Code actions (if present)
// };

// /// \ingroup lsp_code_action_types
// /// \brief Code action disabled reason
// ///
// /// Explains why a code action is disabled.
// struct lsp_code_action_disabled {
//     lsp_string reason;  ///< Reason why action is disabled
// };

// /// \ingroup lsp_document_types
// /// \brief Text document position parameters
// ///
// /// Common parameters for operations that require both a text document
// /// and a position within that document.
// struct lsp_text_document_position_params {
//     struct lsp_text_document_identifier text_document;  ///< Document identifier
//     lsp_position position;                               ///< Position within document
// };



/// \ingroup lsp_document_types
/// \brief Prepare rename result kind enumeration
///
/// Defines the possible types of results for prepare rename requests.
/// The result can be null, a range, or a range with placeholder text.
enum lsp_prepare_rename_result_kind {
    LSP_PREP_RENAME_NONE = 0,              ///< No rename possible
    LSP_PREP_RENAME_RANGE,                 ///< Simple range result
    LSP_PREP_RENAME_RANGE_WITH_PLACEHOLDER ///< Range with placeholder text
};



/// \defgroup lsp_code_lens_types Code Lens Types
/// \brief Types for code lens requests and responses
///
/// Types used specifically for code lens functionality.


/// \defgroup lsp_document_types Document Types
/// \brief Types for text document operations and identification
///
/// Types used specifically for text document handling, identification,
/// and operations across various language features.

/// \ingroup lsp_document_types
/// \brief Reason for text document save
///
/// Specifies why a text document was saved, which affects
/// how the save operation should be processed.
enum lsp_text_document_save_reason {
    LSP_SAVE_REASON_MANUAL = 1,       ///< Manual save (user triggered)
    LSP_SAVE_REASON_AFTER_DELAY = 2,  ///< Auto-save after delay
    LSP_SAVE_REASON_FOCUS_OUT = 3     ///< Auto-save on focus loss
};



// /// \ingroup lsp_document_types
// /// \brief Versioned text document identifier
// ///
// /// Identifies a specific version of a text document.
// /// Used when operations need to reference a particular document version.
// struct lsp_versioned_text_document_id {
//     lsp_uri uri;    ///< Document URI
//     int version;    ///< Document version number
// };





/// \ingroup lsp_document_types
/// \brief Link between locations
///
/// Represents a link between an origin location and a target location.
/// Provides more detailed navigation information than a simple location.
struct lsp_location_link {
    lsp_range origin_selection_range;  ///< Range of origin selection (context)
    lsp_uri target_uri;                ///< Target resource URI
    lsp_range target_range;            ///< Target range in resource
    lsp_range target_selection_range;  ///< Target selection range
};

/// \ingroup lsp_document_types
/// \brief Definition result type discriminant
///
/// Defines the possible types of results for definition requests.
/// The result can be null, a single location, or a list of location links.
enum lsp_definition_result_kind {
    LSP_DEF_NONE = 0,              ///< No definition found
    LSP_DEF_LOCATION,              ///< Single location result
    LSP_DEF_LOCATION_LINK_LIST     ///< List of location links
};





/// \ingroup lsp_document_types
/// \brief Optional list of locations
///
/// Represents either an array of locations or null.
/// Used for results that may contain multiple locations or no locations.
struct lsp_location_list {
    int kind;  ///< LSP_OPT_NONE (null) or LSP_OPT_SOME (array)
    struct lsp_list<struct lsp_location>* head;  ///< Head of location list (valid if kind == SOME)
    int count; ///< Number of locations (valid if kind == SOME)
};



// /// \ingroup lsp_document_types
// /// \brief Text edit operation
// ///
// /// Represents a single text edit operation that can be applied
// /// to modify document content.
// struct lsp_text_edit {
//     lsp_range range;      ///< Range to replace
//     lsp_string new_text;  ///< Text to insert
// };


/// \ingroup lsp_color_types
/// \brief Color presentation
///
/// Represents a way to present a color in the document.
/// Contains the label and optional text edits to apply the presentation.
struct lsp_color_presentation {
    lsp_string label;                            ///< Presentation label
    lsp_opt_string text_edit_present;            ///< Whether text edit is present
    struct lsp_text_edit text_edit;              ///< Text edit (valid if present)
    struct lsp_text_edit_list additional_text_edits;  ///< Additional text edits
};





/// \ingroup lsp_color_types
/// \brief Color information
///
/// Associates a color with a specific range in the document.
struct lsp_color_information {
    lsp_range range;       ///< Range where color appears
    struct lsp_color color; ///< The color at this range
};



/// \defgroup lsp_document_link_types Document Link Types
/// \brief Types for document link requests and responses
///
/// Types used specifically for document link functionality.


/// \defgroup lsp_formatting_types Formatting Types
/// \brief Types for formatting requests and responses
///
/// Types used specifically for document formatting functionality.


/// \defgroup lsp_selection_range_types Selection Range Types
/// \brief Types for selection range requests and responses
///
/// Types used specifically for selection range operations.

/// \ingroup lsp_selection_range_types
/// \brief Selection range structure
///
/// Represents a single selection range with an optional parent.
/// Selection ranges form a hierarchy where each range can have a parent
/// representing a larger selection context.
struct lsp_selection_range {
    lsp_range range;                           ///< The range of this selection
    struct lsp_selection_range* parent;        ///< Optional parent selection range
};

/// \ingroup lsp_selection_range_types
/// \brief Selection range list
///
/// Contains a list of selection ranges returned as the result
/// of a selection range request.
struct lsp_selection_range_list {
    struct lsp_list<struct lsp_selection_range>* head;  ///< List of selection ranges
    int count;                                           ///< Number of selection ranges
};

/// \ingroup lsp_selection_range_types
/// \brief Selection range parameters
///
/// Parameters for a selection range request.
/// Contains the text document identifier and positions for which selection ranges should be computed.
struct lsp_selection_range_params {
    struct lsp_text_document_identifier text_document;  ///< Document to analyze
    struct lsp_list<lsp_position>* positions;           ///< Positions to compute ranges for
};

/// The `textDocument/selectionRange` request is sent from the client to the server to compute selection ranges at given positions.
/// The request's parameter is of type \ref lsp_selection_range_params and the response is of type \ref lsp_selection_range or a Thenable that resolves to such.
/// \since 3.15.0
struct lsp_text_document_selection_range_request {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Selection range parameters.
    struct lsp_selection_range_params params;
};
/// \brief Final selection ranges response.
struct lsp_text_document_selection_range_response {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief Array of selection ranges.
    struct lsp_selection_range_list result;
};
/// \brief Error result for selection range request.
struct lsp_text_document_selection_range_error_result {
    /// \brief Common message header.
    struct lsp_msg_hdr hdr;
    /// \brief JSON-RPC error fields.
    struct lsp_error_result error;
};
static struct lsp_text_document_selection_range_request*      lsp_init_text_document_selection_range_request(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_selection_range_response*     lsp_init_text_document_selection_range_response(void *mem, AkU64 tag_id) noexcept;
static struct lsp_text_document_selection_range_error_result* lsp_init_text_document_selection_range_error_result(void *mem, AkU64 tag_id) noexcept;


