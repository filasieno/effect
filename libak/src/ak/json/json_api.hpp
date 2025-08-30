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
    
    struct JSONParseSession;

    struct ParseHandlers {
        Void (*object_begin)(JSONParseSession* session) = nullptr;
        Void (*object_end)(JSONParseSession* session) = nullptr;
        Void (*array_begin)(JSONParseSession* session) = nullptr;
        Void (*array_end)(JSONParseSession* session) = nullptr;
        Void (*attr_begin)(JSONParseSession* session) = nullptr;
        Void (*attr_key_begin)(JSONParseSession* session) = nullptr;
        Void (*attr_key_end)(JSONParseSession* session) = nullptr;
        Void (*attr_key_chars)(JSONParseSession* session, const Char* str, Size len) = nullptr;
        Void (*key)(JSONParseSession* session, const Char* str, Size len) = nullptr;
        Void (*attr_end)(JSONParseSession* session) = nullptr;
        Void (*null_value)(JSONParseSession* session) = nullptr;
        Void (*bool_value)(JSONParseSession* session, Bool value) = nullptr;
        Void (*int_value)(JSONParseSession* session, I64 value) = nullptr;
        Void (*float_value)(JSONParseSession* session, F64 value) = nullptr;
        Void (*string_value_begin)(JSONParseSession* session) = nullptr;
        Void (*string_value_end)(JSONParseSession* session) = nullptr;
        Void (*string_value_chars)(JSONParseSession* session, const Char* str, Size len) = nullptr;
        Void (*string)(JSONParseSession* session, const Char* str, Size len) = nullptr;
        Void (*parse_state_changed)(JSONParseSession* session) = nullptr;
    };

    struct JSONParseContext;    
    struct JSONParseSession;

    ///\brief Define the Continuation state routine
    using JSONParserStateFn = JSONParserState(JSONParseSession* session, U32 sub_state, Char* head, Char* end, U64 json_size, U64 string_size) noexcept;

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
        ParseHandlers          handlers;            ///< User's installed for this parse session
        void*                  parser_buffer;       ///< The buffer that holds the unaligned parser
        U64                    parser_buffer_size;  ///< The size of the buffer that holds the unaligned parser
        
        Char*                  buffer;              ///< Current input buffer
        Size                   buffer_len;          ///< Length of the buffer
        JSONParserState        state;               ///< The current state of the parser
        U32                    sub_state;           ///< The current sub-state of the parser
        U64                    json_offset;         ///< Number of bytes parsed in the JSON data
        U64                    string_offset;       ///< Number of bytes parsed in a string
        const Char*            err_msg;             ///< Static error message

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
    JSONParseSession* init_json_parser(Void* parser_buffer, U64 parser_buffer_size, const JSONParseSessionConfig* cfg, const ParseHandlers* handlers, Void* user_data) noexcept;

    ///\brief Parse the JSON data
    ///\param session The session to parse
    ///\return The parser state
    JSONParserState parse_buffer(JSONParseSession* session, Void* buffer, U64 buffer_size) noexcept;

    ///\brief Reset the JSON parser
    ///\param session The parser to reset
    Void reset_json_parse_session(JSONParseSession* session) noexcept;

} // namespace ak


