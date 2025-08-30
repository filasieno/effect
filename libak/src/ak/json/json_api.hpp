#pragma once

#include <ak/base/base_api.hpp> // Assuming this includes necessary types like Void, Size, Bool, etc.

namespace ak {

    enum class JSONParserState {
        INVALID,      // Invalid state, e.g., after an error
        INITIALIZED,  // Parser is ready to start
        CONTINUE,     // Parsing can continue with more data
        DONE,         // Parsing completed successfully
        ERROR         // An error occurred during parsing
    };

    // Numeric error codes for JSON parser (200-series)
    // Groups:
    // 200-209: Framework/internal
    // 210-219: Root/structure expectations
    // 220-239: Object/array punctuation and structure
    // 240-259: Strings and escapes
    // 260-269: Numbers
    // 270-279: Keywords (true/false/null)
    // 290-299: Limits and overflow
    enum class JSONErrorCode : U32 {
        NONE                                 = 0,
        FATAL_STACK_OOB                      = 200,
        STACK_OVERFLOW_ON_SUSPEND            = 201,

        EMPTY_INPUT                          = 210,
        UNEXPECTED_EOF                       = 211,
        EXPECTED_OBJECT_OR_ARRAY             = 212,

        EXPECTED_COMMA_OR_CLOSING_BRACE      = 220,
        EXPECTED_COMMA_OR_CLOSING_BRACKET    = 221,
        EXPECTED_VALUE_AFTER_COMMA           = 222,
        EXPECTED_STRING_KEY                  = 223,
        EXPECTED_COLON_AFTER_KEY             = 224,
        UNEXPECTED_CHAR_IN_VALUE             = 225,

        INVALID_ESCAPE_CHAR                  = 240,
        INVALID_UNICODE_HEX_DIGIT            = 241,
        INVALID_SURROGATE_PAIR               = 242,

        NUMBER_TOO_LONG                      = 260,
        INVALID_NUMBER_FORMAT                = 261,
        LEADING_ZERO_NOT_ALLOWED             = 262,
        NO_DIGITS_AFTER_DECIMAL              = 263,
        NO_DIGITS_IN_EXPONENT                = 264,
        INVALID_INTEGER_FORMAT               = 265,
        INVALID_FLOAT_FORMAT                 = 266,

        INVALID_TOKEN_EXPECTED_NULL          = 270,
        INVALID_TOKEN_EXPECTED_TRUE          = 271,
        INVALID_TOKEN_EXPECTED_FALSE         = 272,
    };
    
    struct JSONParseSession;

    enum class JSONEvent {
        OBJECT_BEGIN,
        OBJECT_END,
        ARRAY_BEGIN,
        ARRAY_END,
        ATTR_BEGIN,
        ATTR_KEY_BEGIN,
        ATTR_KEY_END,
        ATTR_KEY_CHARS,
        KEY,
        ATTR_END,
        NULL_VALUE,
        BOOL_VALUE,
        INT_VALUE,
        FLOAT_VALUE,
        STRING_VALUE_BEGIN,
        STRING_VALUE_END,
        STRING_VALUE_CHARS,
        STRING,
        PARSE_STATE_CHANGED
    };

    union JSONEventData {
        struct {
            const Char* str;
            Size len;
        } string_data;

        Bool bool_value;
        I64 int_value;
        F64 float_value;

        struct {
            JSONParserState state;
            U32 err_code;
        } state_data;
    };

    struct JSONParseContext;    
    struct JSONParseSession;

    ///\brief Define the Continuation state routine
    using JSONParserStateFn = JSONParserState(JSONParseSession* session, U32 sub_state, Char* head, Char* end, U64 json_size, U64 string_size) noexcept;

    ///\brief Unified event callback function type
    using JSONParserCallbackFn = Void(JSONParseSession* session, JSONEvent event, const JSONEventData* data) noexcept;

    ///\brief The JSON parse context
    struct JSONParseContext {
        JSONParserStateFn* continuation;                
        void*   user_data;
        U32     sub_state;        
        U32     _reserved;
    };
    static_assert(sizeof(JSONParseContext) == 24, "JSONParseContext must be 32 bytes");
    
    ///\brief Configuration for the JSON parse session
    struct JSONParseSessionConfig {
        U64 max_json_size   = 1024 * 1024;  ///< Maximum size of the JSON data (defaults to 1Mb)
        U64 max_string_size = 2048;         ///< Maximum size of the string (defaults to 2048)
        U32 max_depth       = 32;           ///< Maximum depth of the JSON structure
    };

    ///\brief The JSON parse session
    struct JSONParseSession {
        JSONParseSessionConfig config;              ///< Contains the users configuration parameters
        Void*                  user_data;           ///< User data passed to the handlers
        JSONParserCallbackFn*  on_event; ///< Unified event callback
        void*                  parser_buffer;       ///< The buffer that holds the unaligned parser
        U64                    parser_buffer_size;  ///< The size of the buffer that holds the unaligned parser
        
        Char*                  buffer;              ///< Current input buffer
        Size                   buffer_len;          ///< Length of the buffer
        JSONParserState        state;               ///< The current state of the parser
        U32                    sub_state;           ///< The current sub-state of the parser
        U64                    json_offset;         ///< Number of bytes parsed in the JSON data
        U64                    string_offset;       ///< Number of bytes parsed in a string
        U32                    err_code;            ///< Numeric error code when state==ERROR

        JSONParseContext*      stack_begin;         ///< Points to the first element of the stack
        JSONParseContext*      stack_end;           ///< Points past the last element of the stack
        JSONParseContext*      stack_top;           ///< Points to the next

        ///\brief Partial parse buffer used to save partial number values for instance.
        ///\details if the suspend buffer is 
        char                   suspend_buffer[128]; 
        U64                    suspend_buffer_size;

    };

    ///\brief Get the required buffer size for the JSON parse session
    ///\param cfg Configuration for the JSON parse session
    ///\return The required buffer size
    U64 get_required_parse_session_buffer_size(JSONParseSessionConfig* cfg) noexcept;

    ///\brief Initialize the JSON parse session
    ///\param buffer      the block of memory that will hold parser
    ///\param buffer_size the size of the block of memory that will hold the parser 
    ///\param handlers    the handlers to use
    ///\return The Initialized parse session or nullptr if the session could not be initialized
    JSONParseSession* init_json_parser(Void* parser_buffer, U64 parser_buffer_size, const JSONParseSessionConfig* cfg, JSONParserCallbackFn* on_event, Void* user_data) noexcept;

    ///\brief Parse the JSON data
    ///\param session The session to parse
    ///\return The parser state
    JSONParserState parse_buffer(JSONParseSession* session, Void* buffer, U64 buffer_size) noexcept;

    ///\brief Reset the JSON parser
    ///\param session The parser to reset
    Void reset_json_parse_session(JSONParseSession* session) noexcept;

} // namespace ak


