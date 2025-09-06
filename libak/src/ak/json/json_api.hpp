#pragma once

#include <ak/base/base_api.hpp> // Assuming this includes necessary types like void, AkSize, bool, etc.

struct AkJSONParser;
struct AkJSONParserCtx;    

enum class AkJSONParserState {
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
enum class AkJSONErrorCode : AkU32 {
    NONE                                 = 0,
    FATAL_STACK_OOB                      = 200,
    STACK_OVERFLOW_ON_SUSPEND            = 201,
    INVALID_ARGUMENT                     = 202,
    MISSING_CALLBACK                     = 203,
    USER_ABORTED                         = 204,

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
    FLOAT_TOO_MANY_DIGITS                = 267,

    INVALID_TOKEN_EXPECTED_NULL          = 270,
    INVALID_TOKEN_EXPECTED_TRUE          = 271,
    INVALID_TOKEN_EXPECTED_FALSE         = 272,
    
    // Limits / overflow
    MAX_DEPTH_EXCEEDED                   = 290,
};

enum class AkJSONEvent {
    OBJECT_BEGIN,
    OBJECT_END,
    ARRAY_BEGIN,
    ARRAY_END,
    ATTR_KEY,
    NULL_VALUE,
    BOOL_VALUE,
    INT_VALUE,
    FLOAT_VALUE,
    STRING_VALUE,
    PARSE_STATE_CHANGED,
    PARSE_EOF
};

union AkJSONEventData {
    struct {
        const char* str;
        AkSize len;
    } string_data;

    bool bool_value;
    AkI64 int_value;
    AkF64 float_value;

    struct {
        AkJSONParserState state;
        AkU32 err_code;
    } state_data;
};

///\brief Define the Continuation state routine
using AkJSONParserStateFn = AkJSONParserState(AkJSONParser* session, AkU32 sub_state, char* head, char* end, AkU64 json_size, AkU64 string_size) noexcept;

///\brief Unified event callback function type
///\details Returns 0 to continue parsing; non-zero to abort with USER_ABORTED error.
using AkJSONParserCallbackFn = int(AkJSONParser* session, AkJSONEvent event, const AkJSONEventData* data, AkU64 more) noexcept;

///\brief The JSON parse context
struct AkJSONParserCtx {
    AkJSONParserStateFn* continuation;                
    void*   user_data;
    AkU32     sub_state;        
    AkU32     _reserved;
};
static_assert(sizeof(AkJSONParserCtx) == 24, "JSONParseContext must be 32 bytes");

///\brief Configuration for the JSON parse session
struct AkJSONParserConfig {
    AkU64 max_json_size   = 1024 * 1024;  ///< Maximum size of the JSON data (defaults to 1Mb)
    AkU64 max_string_size = 2048;         ///< Maximum size of the string (defaults to 2048)
    AkU32 max_depth       = 32;           ///< Maximum depth of the JSON structure
};

///\brief The JSON parse session
struct AkJSONParser {
    AkJSONParserConfig       config;              ///< Contains the users configuration parameters
    void*                  user_data;           ///< Original user session context
    AkJSONParserCallbackFn*  on_event;            ///< Unified event callback
    void*                  parser_buffer;       ///< The buffer that holds the unaligned parser
    AkU64                    parser_buffer_size;  ///< The size of the buffer that holds the unaligned parser
    
    char*                  buffer;              ///< Current input buffer
    AkSize                   buffer_len;          ///< Length of the current input buffer
    AkJSONParserState        state;               ///< The current state of the parser
    AkU32                    sub_state;           ///< The current sub-state of the parser
    AkU64                    json_offset;         ///< Number of bytes parsed in the JSON data
    AkU64                    string_offset;       ///< Number of bytes parsed in a string
    AkU32                    err_code;            ///< Numeric error code when state==ERROR

    AkJSONParserCtx*         stack_begin;         ///< Points to the first element of the stack
    AkJSONParserCtx*         stack_end;           ///< Points past the last element of the stack
    AkJSONParserCtx*         stack_top;           ///< Points to the next

    ///\brief Partial parse buffer used to save partial number values for instance.
    ///\details if the suspend buffer is 
    char                   suspend_buffer[128]; 
    AkU64                    suspend_buffer_size;

};

///\brief Get the required buffer size for the JSON parse session
///\param cfg Configuration for the JSON parse session
///\return The required buffer size
AkU64             ak_get_required_buffer_size(AkJSONParserConfig* cfg) noexcept;

///\brief Initialize a JSON Parser
///\param parser_buffer      the block of memory that will hold parser
///\param parser_buffer_size the size of the block of memory that will hold the parser 
///\param cfg                the configuration for the JSON parse session
///\param on_event           the event callback function
///\param user_data          the initial user data    
///\return The Initialized parse session or nullptr if the session could not be initialized
AkJSONParser*     ak_init_json_parser(void *buffer, AkU64 buffer_size, AkJSONParserConfig *cfg, AkJSONParserCallbackFn* on_event, void *user_data) noexcept;

///\brief Parse the JSON data
///\param session The session to parse
///\return The parser state
AkJSONParserState ak_run_json_parser(AkJSONParser* parser, void* buffer, AkU64 buffer_size) noexcept;

///\brief Marks the end of file for the JSON data
///\param session the active parse session
///\return The parser state
AkJSONParserState ak_eof_json_parser(AkJSONParser* parser) noexcept;

///\brief Reset the JSON parser
///\param session The parser to reset
void            ak_reset_json_parser(AkJSONParser* parser) noexcept;




