#include "ak/json/json.hpp" // IWYU pragma: keep

#include <cstring>
#include <cstdlib>
#include <charconv>
#include <system_error>


/// \brief **JSON parser: tail-recursive, streaming, suspendable**
/// \internal
///
/// Architecture:
///
/// - Tail-recursive state machine: each state consumes input and tail-calls the next.
///   Continuations are stored explicitly on an internal stack (JSONParseContext[]);
///   the C++ call stack is never used for parsing recursion.
/// - Streaming: run_json_parser() can be called repeatedly with successive buffers.
///   The parser keeps offsets and a tiny suspend buffer (numbers only).
/// - One-character lookahead with fast character-class tables to reduce branches.
///
/// Callbacks and events (SAX-like):
///
/// - Emitted through unified JSONEvent callbacks stored in the session.
/// - Objects/arrays emit begin/end; keys/strings use fast-path single callback when
///   contained in one buffer, or streaming triplets (begin/chars/end) otherwise.
///
/// Strings:
///
/// - Validate escapes (\", \\, \/, \\b, \f, \\n, \\r, \\t) and \\uXXXX (including surrogate pairs).
/// - Do not decode; validated UTF-8 is forwarded as-is to callbacks.
/// - Spanning across buffers switches to streaming callbacks.
///
/// UTF-8 emission (why a callback):
///
/// - A small UTF-8 encoder emits bytes via a callback so one encoder targets
///   multiple sinks (key/value) without code duplication or hot-path branching.
///   The indirection is negligible vs I/O and keeps state code concise.
///
/// Numbers:
///
/// - Only token using suspend_buffer to span buffers; format validated per RFC 8259.
///
/// Errors and contracts:
///
/// - Public API validates arguments at runtime and returns error/null on invalid input.
/// - Programmer errors are guarded by AK_ASSERT in debug builds.
/// - raise_error() sets ERROR state and err_code, emits PARSE_STATE_CHANGED, and stops.
///
/// Design choices:
///
/// - Character-class tables: reduce branches and enable compact, cache-friendly classification.
/// - Explicit continuation stack: enables suspend/resume with precise state capture; depth checks
///   raise runtime errors for user misconfiguration.
/// - Public API checks vs assertions: user errors -> errors; programmer errors -> asserts.
/// - Suspend buffer asymmetry (numbers only): numbers are the only token that may need to be
///   reconstructed across buffers as a contiguous lexical unit to validate the grammar before
///   deciding INT vs FLOAT. Strings are instead emitted verbatim to the client via chunked
///   callbacks (with the 'more' flag) so the client can choose its own decoding strategy
///   (e.g., UTF-8 validation/decoding, allocation policy). We deliberately avoid buffering
///   strings internally to not impose an allocation/decoding policy on users.
/// - Character-class tables: keeping the tables compact and focused on first-byte dispatch
///   yields the best cost/benefit. Adding higher-level semantic tags would bloat tables without
///   removing meaningful branches in state code, so we keep them minimal and fast.
/// - The 'more' parameter in callbacks is mandatory. It indicates whether additional chunks
///   for the current key/string value will follow. API consumers must rely on it to know
///   when a streaming text emission is complete. See json_api.hpp for precise semantics.
///
/// TODO (next version):
///
/// - SIMD/SWAR whitespace skipping and first-byte classification (AVX2/NEON).
/// - Table-driven separators in object/array states to reduce switches.
/// - Batching small string/number callbacks to amortize call overhead under streaming.
/// - Cache/layout tuning (align session/stack; separate hot/cold fields).
/// - Optional templated emit path to enable full inlining where beneficial.
///
/// File layout:
///
/// - Utilities (tables, helpers, notify wrappers)
/// - Public API
/// - State routine declarations
/// - State routine implementations

// ==========================================
// Utilities - forward declarations
// ==========================================

enum CharClass : AkU8 {
    CHAR_OTHER = 0,
    CHAR_WHITESPACE = 1,
    CHAR_QUOTE = 2,
    CHAR_COMMA = 3,
    CHAR_MINUS = 4,
    CHAR_DIGIT = 5,
    CHAR_COLON = 6,
    CHAR_LBRACKET = 7,
    CHAR_RBRACKET = 8,
    CHAR_F = 9,
    CHAR_N = 10,
    CHAR_T = 11,
    CHAR_LBRACE = 12,
    CHAR_RBRACE = 13
};

/// \brief Callback type for streaming emission of validated/decoded bytes.
/// \details
/// Rationale: abstract the destination (key/value sinks) to avoid branching
///  in the encoder and code duplication across call sites.
/// \internal
using EmitFn = void(AkJSONParser *session, const char *buf, AkU64 len);

static AkJSONParserState raise_error(AkJSONParser *session, AkJSONErrorCode code) noexcept;
static AkJSONParserState suspend_parser(AkJSONParser *session, AkJSONParserStateFn *fn, AkU32 sub_state, AkU64 json_size, AkU64 string_size) noexcept;

/// \internal Fast char classification utility
static inline CharClass classify_char(char c) noexcept;

/// \brief Encode codepoint to UTF-8 and emit via callback.
/// \details
///  Rationale: reuse one encoder for multiple sinks (key/value) while keeping
///  hot paths simple; function-pointer cost is minimal and localized.
/// \internal
static inline void emit_utf8_bytes(AkJSONParser *session, AkU32 cp, EmitFn *emit);

/// \brief Return true if character belongs to number token class.
/// \internal
static inline bool is_number_char(char c) noexcept;

/// \brief Prefetch static classification tables to warm caches.
/// \internal
static inline void prefetch_classification_tables() noexcept;

/// \brief Skip whitespace and advance json_size accordingly.
/// \internal
static inline char* skip_whitespace(char* head, char* end, AkU64* json_size) noexcept;

/// Shared escape/Unicode helpers

///\brief Parse exactly 4 hexadecimal digits from a JSON \uXXXX sequence.
///\details
/// - Advances head and json_size on success
/// - Returns false when buffer ends (caller should suspend)
/// - Signals an error via raise_error(session, ...) on invalid hex digit and returns false
/// \internal
static inline bool parse_hex4(AkJSONParser *session, char **phead, char *end, AkU64 *pjson_size, AkU32 *pout) noexcept;

/// \brief Unified event notification
/// \internal
static void notify_event(AkJSONParser *session, AkJSONEvent event_type, const AkJSONEventData *data = nullptr, AkU64 more = 0) noexcept;
static inline bool push_parse_context_checked(AkJSONParser *session, AkJSONParserStateFn *fn, AkU32 sub_state = 0) noexcept;
static AkJSONParserState resume_parse_context(AkJSONParser *session, AkU32 sub_state, char *head, char *end, AkU64 json_size, AkU64 string_size) noexcept;

// ==========================================
// Constants and Tables
// ==========================================

// Configurable policy: maximum significant decimal digits accepted for floats before erroring
// Rationale: controls mantissa precision to avoid overlong textual inputs creating surprising
// rounding; adjust as needed for your application. Kept in this .cpp for easy tuning.
static constexpr AkU32 MAX_FLOAT_SIGNIFICANT_DIGITS = 16;

// UTF-8 encoding constants
static constexpr AkU32 UTF8_1_MAX           = 0x80U;
static constexpr AkU32 UTF8_2_MAX           = 0x800U;
static constexpr AkU32 SURROGATE_HIGH_START = 0xD800U;
static constexpr AkU32 SURROGATE_HIGH_END   = 0xDBFFU;
static constexpr AkU32 SURROGATE_LOW_START  = 0xDC00U;
static constexpr AkU32 SURROGATE_LOW_END    = 0xDFFFU;

// Character classification lookup table for fast dispatch
static constexpr AkU8 char_class_table[256] = {
    // 0x00-0x0F: control
    0,0,0,0,0,0,0,0,0,1,1,0,0,1,0,0,
    // 0x10-0x1F: control
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    // 0x20-0x2F: space, punctuation
    1,0,2,0,0,0,0,0,0,0,0,0,3,4,0,0,
    // 0x30-0x3F: digits, colon
    5,5,5,5,5,5,5,5,5,5,6,0,0,0,0,0,
    // 0x40-0x4F
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    // 0x50-0x5F (include '[' and ']')
    0,0,0,0,0,0,0,0,0,0,0,7,0,8,0,0,
    // 0x60-0x6F (include 'f'=0x66, 'n'=0x6E)
    0,0,0,0,0,0,9,0,0,0,0,0,0,0,10,0,
    // 0x70-0x7F (include 't'=0x74, '{'=0x7B, '}'=0x7D)
    0,0,0,0,11,0,0,0,0,0,0,12,0,13,0,0,
    // 0x80-0x8F
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    // 0x90-0x9F
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    // 0xA0-0xAF
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    // 0xB0-0xBF
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    // 0xC0-0xCF
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    // 0xD0-0xDF
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    // 0xE0-0xEF
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    // 0xF0-0xFF
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

// Number character classification (for numbers: '-', '+', '.', 'e', 'E', '0'-'9')
static constexpr AkU8 number_char_table[256] = {
    // 0x00-0x2F
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,1,0,1,1,0, // '+' (0x2B)=1, ',' (0x2C)=0, '-' (0x2D)=1, '.' (0x2E)=1, '/' (0x2F)=0
    // 0x30-0x3F
    1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0, // '0'-'9'
    // 0x40-0x4F
    0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0, // 'E' (0x45)=1
    // 0x50-0x5F
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    // 0x60-0x6F
    0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0, // 'e' (0x65)=1
    // 0x70-0x7F
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    // 0x80-0xFF (unused for number syntax)
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};


// ==========================================
// State routines - forward declarations
// ==========================================

// Parser state changed
static void notify_state_changed(AkJSONParser *session) noexcept;

// Literal Values
static void notify_value_null(AkJSONParser *session) noexcept;
static void notify_value_bool(AkJSONParser *session, bool value) noexcept;
static void notify_value_number_int(AkJSONParser *session, AkI64 value) noexcept;
static void notify_value_number_float(AkJSONParser *session, AkF64 value) noexcept;
static void notify_value_string_chunk(AkJSONParser *session, const char *text_buffer, AkU64 text_buffer_length, AkU64 more) noexcept;
static void notify_value_string(AkJSONParser *session, const char *text_buffer, AkU64 text_buffer_length) noexcept;

// Array  Notification
static void notify_array_begin(AkJSONParser *session) noexcept;
static void notify_array_end(AkJSONParser *session) noexcept;

// Object  Notification
static void notify_object_begin(AkJSONParser *session) noexcept;
static void notify_object_end(AkJSONParser *session) noexcept;

// Attribute Notification
static void notify_attr_key_chunk(AkJSONParser *session, const char *text_buffer, AkU64 text_buffer_length, AkU64 more) noexcept;
static void notify_attr_key(AkJSONParser *session, const char *text_buffer, AkU64 text_buffer_length) noexcept;
static void emit_attr_key_utf8(AkJSONParser *session, const char *text_buffer, AkU64 text_buffer_length) noexcept;

// ==========================================
// State function declarations
// ==========================================

static AkJSONParserState sentinel(AkJSONParser *session, AkU32 sub_state, char *head, char *end, AkU64 json_size, AkU64 string_size) noexcept;
static AkJSONParserState state_return_result(AkJSONParser *session, AkU32 sub_state, char *head, char *end, AkU64 json_size, AkU64 string_size) noexcept;
static AkJSONParserState state_root_dispatch(AkJSONParser *session, AkU32 sub_state, char *head, char *end, AkU64 json_size, AkU64 string_size) noexcept;

static AkJSONParserState state_object_first_attr(AkJSONParser *session, AkU32 sub_state, char *head, char *end, AkU64 json_size, AkU64 string_size) noexcept;
static AkJSONParserState state_object_rest_attrs(AkJSONParser *session, AkU32 sub_state, char *head, char *end, AkU64 json_size, AkU64 string_size) noexcept;

static AkJSONParserState state_array_first_value(AkJSONParser *session, AkU32 sub_state, char *head, char *end, AkU64 json_size, AkU64 string_size) noexcept;
static AkJSONParserState state_list_rest_values(AkJSONParser *session, AkU32 sub_state, char *head, char *end, AkU64 json_size, AkU64 string_size) noexcept;
static AkJSONParserState state_array_value_required(AkJSONParser *session, AkU32 sub_state, char *head, char *end, AkU64 json_size, AkU64 string_size) noexcept;

static AkJSONParserState state_attr_begin_key(AkJSONParser *session, AkU32 sub_state, char *head, char *end, AkU64 json_size, AkU64 string_size) noexcept;
static AkJSONParserState state_attr_key_chars(AkJSONParser *session, AkU32 sub_state, char *head, char *end, AkU64 json_size, AkU64 string_size) noexcept;
static AkJSONParserState state_attr_separator(AkJSONParser *session, AkU32 sub_state, char *head, char *end, AkU64 json_size, AkU64 string_size) noexcept;

static AkJSONParserState state_value_dispatch(AkJSONParser *session, AkU32 sub_state, char *head, char *end, AkU64 json_size, AkU64 string_size) noexcept;
static AkJSONParserState state_null_head(AkJSONParser *session, AkU32 sub_state, char *head, char *end, AkU64 json_size, AkU64 string_size) noexcept;
static AkJSONParserState state_true_head(AkJSONParser *session, AkU32 sub_state, char *head, char *end, AkU64 json_size, AkU64 string_size) noexcept;
static AkJSONParserState state_false_head(AkJSONParser *session, AkU32 sub_state, char *head, char *end, AkU64 json_size, AkU64 string_size) noexcept;
static AkJSONParserState state_number_head(AkJSONParser *session, AkU32 sub_state, char *head, char *end, AkU64 json_size, AkU64 string_size) noexcept;
static AkJSONParserState state_string_head(AkJSONParser *session, AkU32 sub_state, char* head, char* end, AkU64 json_size, AkU64 string_size) noexcept;

// ==========================================
// Public function implementation
// ==========================================

AkJSONParser *ak_init_json_parser(void *buffer, AkU64 buffer_size, AkJSONParserConfig *cfg, AkJSONParserCallbackFn* on_event, void *user_data) noexcept {
    if (buffer == nullptr || cfg == nullptr || on_event == nullptr) {
        return nullptr;
    }
    if (buffer_size < sizeof(AkJSONParser)) {
        return nullptr;
    }

    if constexpr (AK_IS_DEBUG_MODE) {
        std::memset(buffer, 0, buffer_size);
    }

    AkU64 required_size = ak_get_required_buffer_size(cfg);

    if (buffer_size < required_size) {
        return nullptr;
    }

    AkJSONParser *session = (AkJSONParser *)buffer;

    session->parser_buffer = buffer;
    session->parser_buffer_size = buffer_size;

    session->stack_begin = (AkJSONParserCtx *)((char *)session + sizeof(AkJSONParser));
    session->stack_end = (AkJSONParserCtx *)((char *)session->stack_begin + (cfg->max_depth * (AkU64)sizeof(AkJSONParserCtx)));
    session->stack_top = session->stack_begin;

    session->config = *cfg;
    session->on_event = on_event;
    session->state = AkJSONParserState::INITIALIZED;

    session->buffer = nullptr;
    session->buffer_len = 0;
    session->json_offset = 0;
    session->string_offset = 0;
    session->user_data = user_data;
    session->suspend_buffer_size = 0;

    // Prefetch tables and notify that the parser has been initialized
    prefetch_classification_tables();
    notify_state_changed(session);

    return session;
}

void ak_reset_json_parser(AkJSONParser *session) noexcept {
    AK_ASSERT(session != nullptr);
    if (session == nullptr) {
        return;
    }

    session->stack_top = session->stack_begin;
    session->buffer = nullptr;
    session->buffer_len = 0;
    session->state = AkJSONParserState::INITIALIZED;
    session->json_offset = 0;
    session->string_offset = 0;
}


AkJSONParserState ak_run_json_parser(AkJSONParser *session, void *buffer, AkU64 buffer_size) noexcept {
    AK_ASSERT(session != nullptr);
    if (session == nullptr || buffer == nullptr) {
        return AkJSONParserState::ERROR;
    }
    if (session->state == AkJSONParserState::INVALID) {
        return AkJSONParserState::ERROR;
    }

    // Always set the current buffer for this invocation
    session->buffer = (char *)buffer;
    session->buffer_len = buffer_size;

    if (session->state == AkJSONParserState::CONTINUE) {
        // resume parsing from saved continuation
        AkJSONParserCtx *top_ctx = session->stack_top - 1;
        AkJSONParserStateFn *continuation = top_ctx->continuation;
        return continuation(session, top_ctx->sub_state, session->buffer, session->buffer + session->buffer_len, session->json_offset, session->string_offset);
    }

    if (session->state == AkJSONParserState::INITIALIZED) {
        // first time run, we need to set the initial state
        session->state = AkJSONParserState::CONTINUE;
        session->stack_top = session->stack_begin;
        session->json_offset = 0;
        session->string_offset = 0;
        session->suspend_buffer_size = 0;

        if (!push_parse_context_checked(session, sentinel, 0)) return AkJSONParserState::ERROR;
        if (!push_parse_context_checked(session, state_return_result, 0)) return AkJSONParserState::ERROR;

        return state_root_dispatch(session, 0, session->buffer, session->buffer + session->buffer_len, session->json_offset, session->string_offset);
    }

    // If the current session state is already DONE or ERROR
    return session->state;
}

AkJSONParserState ak_eof_json_parser(AkJSONParser *session) noexcept {
    AK_ASSERT(session != nullptr);
    if (session == nullptr) {
        return AkJSONParserState::ERROR;
    }
    if (session->state == AkJSONParserState::INVALID) {
        return AkJSONParserState::ERROR;
    }

    // Always notify end-of-input
    notify_event(session, AkJSONEvent::PARSE_EOF, nullptr, 0);

    // If already done or error, return current state
    if (session->state == AkJSONParserState::DONE || session->state == AkJSONParserState::ERROR) {
        return session->state;
    }

    // If initialized but never started, this is an error (empty input)
    if (session->state == AkJSONParserState::INITIALIZED) {
        return raise_error(session, AkJSONErrorCode::EMPTY_INPUT);
    }

    // If we're in CONTINUE state, it means the parser is waiting for more input
    // We need to check if the current parsing context can be completed
    if (session->state == AkJSONParserState::CONTINUE) {
        // Check if we have a suspend frame (parser was suspended waiting for more data)
        if (session->stack_top > session->stack_begin) {
            AkJSONParserCtx *top_ctx = session->stack_top - 1;

            // Check if this is a suspend frame (tagged with SUSP_TAG)
            static int SUSP_TAG;
            if (top_ctx->user_data == &SUSP_TAG) {
                // Parser was suspended, which means it was expecting more data
                // Since we're stopping, this is unexpected EOF
                return raise_error(session, AkJSONErrorCode::UNEXPECTED_EOF);
            } else {
                // We have a regular continuation frame
                // This means parsing reached a natural completion point
                // Try to complete by calling the return_state function
                return state_return_result(session, 0, nullptr, nullptr, session->json_offset, 0);
            }
        } else {
            // No frames on stack, but state is CONTINUE - this shouldn't happen
            return raise_error(session, AkJSONErrorCode::FATAL_STACK_OOB);
        }
    }

    // Should not reach here
    return raise_error(session, AkJSONErrorCode::FATAL_STACK_OOB);
}



// ==========================================
// State function implementations
// ==========================================

static AkJSONParserState sentinel(AkJSONParser *session, AkU32 sub_state, char *head, char *end, AkU64 json_size, AkU64 string_size) noexcept {
    (void)(sub_state);
    (void)(head);
    (void)(end);
    (void)(json_size);
    (void)(string_size);

    session->err_code = (AkU32)AkJSONErrorCode::FATAL_STACK_OOB;
    return AkJSONParserState::ERROR;
}

static AkJSONParserState state_return_result(AkJSONParser *session, AkU32 sub_state, char *head, char *end, AkU64 json_size, AkU64 string_size) noexcept {
    (void)(sub_state);
    (void)(head);
    (void)(end);
    (void)(json_size);
    (void)(string_size);

    // If we reached the base return state, parsing is complete
    session->state = AkJSONParserState::DONE;
    notify_state_changed(session);  // Notify state change for successful completion
    return session->state;
}

// Initial and root states
static AkJSONParserState state_root_dispatch(AkJSONParser *session, AkU32 sub_state, char *head, char *end, AkU64 json_size, AkU64 string_size) noexcept {
    head = skip_whitespace(head, end, &json_size);
    if (head == end) {
        if (json_size == 0) return raise_error(session, AkJSONErrorCode::EMPTY_INPUT);
        return raise_error(session, AkJSONErrorCode::UNEXPECTED_EOF);
    }
    CharClass cls = classify_char(*head);
    switch (cls) {
    case CHAR_LBRACE: {
        ++head; ++json_size;
        notify_object_begin(session);
        AK_MUST_TAIL return state_object_first_attr(session, sub_state, head, end, json_size, string_size);
    }
    case CHAR_LBRACKET: {
        notify_array_begin(session);
        ++head; ++json_size;
        AK_MUST_TAIL return state_array_first_value(session, sub_state, head, end, json_size, string_size);
    }
    case CHAR_QUOTE:
    case CHAR_T:
    case CHAR_F:
    case CHAR_N:
    case CHAR_MINUS:
    case CHAR_DIGIT: {
        if (!push_parse_context_checked(session, state_return_result, 0)) return AkJSONParserState::ERROR;
        AK_MUST_TAIL return state_value_dispatch(session, sub_state, head, end, json_size, string_size);
    }
    default:
        return raise_error(session, AkJSONErrorCode::EXPECTED_OBJECT_OR_ARRAY);
    }
}

static AkJSONParserState state_object_first_attr(AkJSONParser *session, AkU32 sub_state, char *head, char *end, AkU64 json_size, AkU64 string_size) noexcept {
    head = skip_whitespace(head, end, &json_size);
    if (head == end)
        return suspend_parser(session, state_object_first_attr, sub_state, json_size, string_size);
    CharClass cls = classify_char(*head);
    if (cls == CHAR_RBRACE) {
        ++head; 
        ++json_size;
        notify_object_end(session);
        AK_MUST_TAIL return resume_parse_context(session, sub_state, head, end, json_size, string_size);
    }
    AK_MUST_TAIL return state_attr_begin_key(session, sub_state, head, end, json_size, string_size);
}

static AkJSONParserState state_object_rest_attrs(AkJSONParser *session, AkU32 sub_state, char *head, char *end, AkU64 json_size, AkU64 string_size) noexcept {
    head = skip_whitespace(head, end, &json_size);
    if (head == end)
        return suspend_parser(session, state_object_rest_attrs, sub_state, json_size, string_size);
    CharClass cls = classify_char(*head);
    if (cls == CHAR_RBRACE) {
        ++head; 
        ++json_size;
        notify_object_end(session);
        AK_MUST_TAIL return resume_parse_context(session, sub_state, head, end, json_size, string_size);
    }
    if (cls == CHAR_COMMA) {
        ++head; 
        ++json_size;
        AK_MUST_TAIL return state_attr_begin_key(session, sub_state, head, end, json_size, string_size);
    }
    return raise_error(session, AkJSONErrorCode::EXPECTED_COMMA_OR_CLOSING_BRACE);
}

static AkJSONParserState state_array_first_value(AkJSONParser *session, AkU32 sub_state, char *head, char *end, AkU64 json_size, AkU64 string_size) noexcept {
    while (true) {
        head = skip_whitespace(head, end, &json_size);
        if (head == end)
            return suspend_parser(session, state_array_first_value, sub_state, json_size, string_size);
        CharClass cls = classify_char(*head);
        if (cls == CHAR_RBRACKET) {
            ++head; ++json_size;
            notify_array_end(session);
            AK_MUST_TAIL return resume_parse_context(session, sub_state, head, end, json_size, string_size);
        }
        if (!push_parse_context_checked(session, state_list_rest_values, 0)) return AkJSONParserState::ERROR;
        AK_MUST_TAIL return state_value_dispatch(session, 0, head, end, json_size, string_size);
    }
}

static AkJSONParserState state_list_rest_values(AkJSONParser *session, AkU32 sub_state, char *head, char *end, AkU64 json_size, AkU64 string_size) noexcept {
    head = skip_whitespace(head, end, &json_size);
    if (head == end)
        return suspend_parser(session, state_list_rest_values, sub_state, json_size, string_size);
    CharClass cls = classify_char(*head);
    if (cls == CHAR_COMMA) {
        ++head; ++json_size;
        if (!push_parse_context_checked(session, state_list_rest_values, 0)) return AkJSONParserState::ERROR;
        AK_MUST_TAIL return state_array_value_required(session, 0, head, end, json_size, string_size);
    }
    if (cls == CHAR_RBRACKET) {
        ++head; ++json_size;
        notify_array_end(session);
        AK_MUST_TAIL return resume_parse_context(session, sub_state, head, end, json_size, string_size);
    }
    return raise_error(session, AkJSONErrorCode::EXPECTED_COMMA_OR_CLOSING_BRACKET);
}

// After a comma inside arrays, a value must follow; ']' is not allowed (catches trailing comma)
static AkJSONParserState state_array_value_required(AkJSONParser *session, AkU32 sub_state, char *head, char *end, AkU64 json_size, AkU64 string_size) noexcept {
    while (true) {
        head = skip_whitespace(head, end, &json_size);
        if (head == end)
            return suspend_parser(session, state_array_value_required, sub_state, json_size, string_size);
        CharClass cls = classify_char(*head);
        if (cls == CHAR_RBRACKET)
            return raise_error(session, AkJSONErrorCode::EXPECTED_VALUE_AFTER_COMMA);
        AK_MUST_TAIL return state_value_dispatch(session, sub_state, head, end, json_size, string_size);
    }
}

// Key string parsing (supports simple escapes and raw chunks)
static AkJSONParserState state_attr_key_chars(AkJSONParser *session, AkU32 sub_state, char *head, char *end, AkU64 json_size, AkU64 string_size) noexcept {
    bool is_complete_key = (sub_state == 1);
    char *key_start = head;
    char *chunk_start = head;

    while (true) {
        if (head == end) {
            if (!is_complete_key && chunk_start != head) {
                notify_attr_key_chunk(session, chunk_start, (AkU64)(head - chunk_start), 1);
                string_size += (AkU64)(head - chunk_start);
            }
            return suspend_parser(session, state_attr_key_chars, 0, json_size, string_size);
        }
        char c = *head;
        if (c == '\\') {
            // On first escape, fall back to streaming mode
            if (is_complete_key) {
                if (chunk_start != head) {
                    notify_attr_key_chunk(session, chunk_start, (AkU64)(head - chunk_start), 1);
                    string_size += (AkU64)(head - chunk_start);
                }
                is_complete_key = false;
            } else if (chunk_start != head) {
                notify_attr_key_chunk(session, chunk_start, (AkU64)(head - chunk_start), 1);
                string_size += (AkU64)(head - chunk_start);
            }
            ++head;
            ++json_size;
            if (head == end) {
                return suspend_parser(session, state_attr_key_chars, 0, json_size, string_size);
            }
            char e = *head;
            ++head;
            ++json_size;
            char rc;
            switch (e) {
            case '"':
                rc = '"';
                break;
            case '\\':
                rc = '\\';
                break;
            case '/':
                rc = '/';
                break;
            case 'b':
                rc = '\b';
                break;
            case 'f':
                rc = '\f';
                break;
            case 'n':
                rc = '\n';
                break;
            case 'r':
                rc = '\r';
                break;
            case 't':
                rc = '\t';
                break;
            case 'u': {
                AkU32 code1;
                if (!parse_hex4(session, &head, end, &json_size, &code1)) {
                    if (session->state == AkJSONParserState::ERROR)
                        return AkJSONParserState::ERROR;
                    return suspend_parser(session, state_attr_key_chars, 0, json_size, string_size);
                }
                if (code1 >= SURROGATE_HIGH_START && code1 <= SURROGATE_HIGH_END) {
                    if (head == end || *head != '\\') {
                        return raise_error(session, AkJSONErrorCode::INVALID_SURROGATE_PAIR);
                    }
                    ++head;
                    ++json_size;
                    if (head == end || *head != 'u') {
                        return raise_error(session, AkJSONErrorCode::INVALID_SURROGATE_PAIR);
                    }
                    ++head;
                    ++json_size;
                    AkU32 code2;
                    if (!parse_hex4(session, &head, end, &json_size, &code2)) {
                        if (session->state == AkJSONParserState::ERROR)
                            return AkJSONParserState::ERROR;
                        return suspend_parser(session, state_attr_key_chars, 0, json_size, string_size);
                    }
                    if (!(code2 >= SURROGATE_LOW_START && code2 <= SURROGATE_LOW_END)) {
                        return raise_error(session, AkJSONErrorCode::INVALID_SURROGATE_PAIR);
                    }
                    AkU32 cp = 0x10000 + (((code1 - 0xD800) & 0x3FF) << 10) + ((code2 - 0xDC00) & 0x3FF);
                    emit_utf8_bytes(session, cp, emit_attr_key_utf8);
                } else if (code1 >= SURROGATE_LOW_START && code1 <= SURROGATE_LOW_END) {
                    return raise_error(session, AkJSONErrorCode::INVALID_SURROGATE_PAIR);
                } else {
                    emit_utf8_bytes(session, code1, emit_attr_key_utf8);
                }
                chunk_start = head;
                continue;
            }
            default:
                return raise_error(session, AkJSONErrorCode::INVALID_ESCAPE_CHAR);
            }
            // Handle the escaped character (streaming mode)
            notify_attr_key_chunk(session, &rc, 1, 1);
            ++string_size;
            chunk_start = head;
            continue;
        } else if (c == '"') {
            // end of key
            if (!is_complete_key && chunk_start != head) {
                notify_attr_key_chunk(session, chunk_start, (AkU64)(head - chunk_start), 1);
                string_size += (AkU64)(head - chunk_start);
            }
            ++head;
            ++json_size;

            if (is_complete_key) {
                // Use optimized callback for complete key
                notify_attr_key(session, key_start, (AkU64)(head - 1 - key_start));
            } else {
                // Finalize chunked key
                notify_attr_key_chunk(session, "", 0, 0);
            }
            AK_MUST_TAIL return state_attr_separator(session, 0, head, end, json_size, string_size);
        } else {
            ++head;
            ++json_size;
            continue;
        }
    }
}

static AkJSONParserState state_attr_begin_key(AkJSONParser *session, AkU32 sub_state, char *head, char *end, AkU64 json_size, AkU64 string_size) noexcept {
    head = skip_whitespace(head, end, &json_size);
    if (head == end) {
        return suspend_parser(session, state_attr_begin_key, sub_state, json_size, string_size);
    }
    if (classify_char(*head) != CHAR_QUOTE) {
        return raise_error(session, AkJSONErrorCode::EXPECTED_STRING_KEY);
    }
    ++head; ++json_size;
    if (!push_parse_context_checked(session, state_object_rest_attrs, 0)) { 
        return AkJSONParserState::ERROR;
    }
    // Pass sub_state=1 to indicate this is a complete key candidate
    AK_MUST_TAIL return state_attr_key_chars(session, 1, head, end, json_size, string_size);
}

// state_attr_end_key not used; end-of-key handled in state_attr_key_chars

static AkJSONParserState state_attr_separator(AkJSONParser *session, AkU32 sub_state, char *head, char *end, AkU64 json_size, AkU64 string_size) noexcept {
    (void)sub_state;
    (void)string_size;
    while (true) {
        head = skip_whitespace(head, end, &json_size);
        if (head == end) {
            return suspend_parser(session, state_attr_separator, 0, json_size, string_size);
        }    
        if (classify_char(*head) != CHAR_COLON) {
            return raise_error(session, AkJSONErrorCode::EXPECTED_COLON_AFTER_KEY);
        }
        ++head; 
        ++json_size;
        AK_MUST_TAIL return state_value_dispatch(session, 0, head, end, json_size, string_size);
    }
}

// Optimized value dispatch using character classes (definition)
static AkJSONParserState state_value_dispatch(AkJSONParser *session, AkU32 sub_state, char *head, char *end, AkU64 json_size, AkU64 string_size) noexcept {
    (void)sub_state;
    (void)string_size;
    head = skip_whitespace(head, end, &json_size);
    if (head >= end)
        return suspend_parser(session, state_value_dispatch, 0, json_size, 0);

    CharClass cls = classify_char(*head);
    switch (cls) {
    case CHAR_N:
        ++head; ++json_size;
        AK_MUST_TAIL return state_null_head(session, 0, head, end, json_size, 0);
    case CHAR_T:
        ++head; ++json_size;
        AK_MUST_TAIL return state_true_head(session, 0, head, end, json_size, 0);
    case CHAR_F:
        ++head; ++json_size;
        AK_MUST_TAIL return state_false_head(session, 0, head, end, json_size, 0);
    case CHAR_QUOTE:
        ++head; ++json_size;
        AK_MUST_TAIL return state_string_head(session, 1, head, end, json_size, 0);
    case CHAR_LBRACE:
        ++head; ++json_size;
        notify_object_begin(session);
        AK_MUST_TAIL return state_object_first_attr(session, 0, head, end, json_size, 0);
    case CHAR_LBRACKET:
        ++head; ++json_size;
        notify_array_begin(session);
        AK_MUST_TAIL return state_array_first_value(session, 0, head, end, json_size, 0);
    case CHAR_MINUS:
    case CHAR_DIGIT:
        session->suspend_buffer_size = 0;
        AK_MUST_TAIL return state_number_head(session, 0, head, end, json_size, 0);
    default:
        return raise_error(session, AkJSONErrorCode::UNEXPECTED_CHAR_IN_VALUE);
    }
}

static AkJSONParserState state_null_head(AkJSONParser *session, AkU32 sub_state, char *head, char *end, AkU64 json_size, AkU64 string_size) noexcept {
    // We already consumed 'n'; now expect 'u' 'l' 'l'
    (void)string_size;
    const char expected[] = {'u', 'l', 'l'};
    AkU32 idx = sub_state; // how many of expected we already matched
    while (idx < 3) {
        if (head == end)
            return suspend_parser(session, state_null_head, idx, json_size, 0);
        char c = *head;
        if (c != expected[idx])
            return raise_error(session, AkJSONErrorCode::INVALID_TOKEN_EXPECTED_NULL);
        ++head;
        ++json_size;
        ++idx;
    }
    notify_value_null(session);
    AK_MUST_TAIL return resume_parse_context(session, 0, head, end, json_size, 0);
}

static AkJSONParserState state_true_head(AkJSONParser *session, AkU32 sub_state, char *head, char *end, AkU64 json_size, AkU64 string_size) noexcept {
    (void)string_size;
    const char expected[] = {'r', 'u', 'e'};
    AkU32 idx = sub_state;
    while (idx < 3) {
        if (head == end)
            return suspend_parser(session, state_true_head, idx, json_size, 0);
        char c = *head;
        if (c != expected[idx])
            return raise_error(session, AkJSONErrorCode::INVALID_TOKEN_EXPECTED_TRUE);
        ++head;
        ++json_size;
        ++idx;
    }
    notify_value_bool(session, true);
    AK_MUST_TAIL return resume_parse_context(session, 0, head, end, json_size, 0);
}

static AkJSONParserState state_false_head(AkJSONParser *session, AkU32 sub_state, char *head, char *end, AkU64 json_size, AkU64 string_size) noexcept {
    (void)string_size;
    const char expected[] = {'a', 'l', 's', 'e'};
    AkU32 idx = sub_state;
    while (idx < 4) {
        if (head == end)
            return suspend_parser(session, state_false_head, idx, json_size, 0);
        char c = *head;
        if (c != expected[idx])
            return raise_error(session, AkJSONErrorCode::INVALID_TOKEN_EXPECTED_FALSE);
        ++head;
        ++json_size;
        ++idx;
    }
    notify_value_bool(session, false);
    AK_MUST_TAIL return resume_parse_context(session, 0, head, end, json_size, 0);
}

// Deprecated: replaced by is_number_char(char)
// static bool is_num_char(char c) noexcept { return (c == '-' || c == '+' || c == '.' || c == 'e' || c == 'E' || (c >= '0' && c <= '9')); }

static AkJSONParserState state_number_head(AkJSONParser *session, AkU32 sub_state, char *head, char *end, AkU64 json_size, AkU64 string_size) noexcept {
    (void)sub_state;
    (void)string_size;
    // collect number into suspend buffer; only use suspend buffer for numbers
    while (true) {
        if (head == end) {
            break; // finalize number at buffer end
        }
        char c = *head;
        if (!is_number_char(c)) {
            break;
        }
        if (session->suspend_buffer_size + 1 < sizeof(session->suspend_buffer)) {
            session->suspend_buffer[session->suspend_buffer_size++] = c;
        } else {
            return raise_error(session, AkJSONErrorCode::NUMBER_TOO_LONG);
        }
        ++head;
        ++json_size;
    }
    // terminate
    session->suspend_buffer[session->suspend_buffer_size] = '\0';
    // Validate number format per RFC 8259
    const char *num = session->suspend_buffer;
    AkU64 len = session->suspend_buffer_size;
    if (len == 0)
        return raise_error(session, AkJSONErrorCode::INVALID_NUMBER_FORMAT);
    AkU64 p = 0;
    if (num[p] == '-') {
        ++p;
        if (p == len)
            return raise_error(session, AkJSONErrorCode::INVALID_NUMBER_FORMAT);
    }
    if (num[p] == '0') {
        // no leading zeros allowed
        if (p + 1 < len && num[p + 1] >= '0' && num[p + 1] <= '9') {
            return raise_error(session, AkJSONErrorCode::LEADING_ZERO_NOT_ALLOWED);
        }
        ++p;
    } else {
        if (!(num[p] >= '1' && num[p] <= '9'))
            return raise_error(session, AkJSONErrorCode::INVALID_NUMBER_FORMAT);
        while (p < len && (num[p] >= '0' && num[p] <= '9'))
            ++p;
    }
    bool is_float = false;
    // Count significant digits to enforce at most MAX_FLOAT_SIGNIFICANT_DIGITS for floats
    AkU32 significant_digits = 0;
    AkU64 q = 0;
    // Re-scan to count significant digits (ignore sign, decimal point, exponent sign)
    while (q < len) {
        char ch = num[q++];
        if (ch == '-' || ch == '+' || ch == '.' || ch == 'e' || ch == 'E') continue;
        if (ch >= '0' && ch <= '9') {
            if (!(significant_digits == 0 && ch == '0')) {
                // count all non-leading-zero digits as significant
                ++significant_digits;
            }
        }
    }
    if (p < len && num[p] == '.') {
        is_float = true;
        ++p;
        if (p == len)
            return raise_error(session, AkJSONErrorCode::NO_DIGITS_AFTER_DECIMAL);
        if (!(num[p] >= '0' && num[p] <= '9'))
            return raise_error(session, AkJSONErrorCode::NO_DIGITS_AFTER_DECIMAL);
        while (p < len && (num[p] >= '0' && num[p] <= '9'))
            ++p;
    }
    if (p < len && (num[p] == 'e' || num[p] == 'E')) {
        is_float = true;
        ++p;
        if (p == len)
            return raise_error(session, AkJSONErrorCode::NO_DIGITS_IN_EXPONENT);
        if (num[p] == '+' || num[p] == '-') {
            ++p;
            if (p == len)
                return raise_error(session, AkJSONErrorCode::NO_DIGITS_IN_EXPONENT);
        }
        if (!(num[p] >= '0' && num[p] <= '9'))
            return raise_error(session, AkJSONErrorCode::NO_DIGITS_IN_EXPONENT);
        while (p < len && (num[p] >= '0' && num[p] <= '9'))
            ++p;
    }
    if (p != len)
        return raise_error(session, AkJSONErrorCode::INVALID_NUMBER_FORMAT);
    // decide int vs float
    for (AkU64 i = 0; i < session->suspend_buffer_size; ++i) {
        char c = session->suspend_buffer[i];
        if (c == '.' || c == 'e' || c == 'E') {
            is_float = true;
            break;
        }
    }
    if (!is_float) {
        // parse integer
        // simple strtoll; we avoid heavy libs; implement minimal
        bool neg = false;
        AkU64 pos = 0;
        AkI64 val = 0;
        if (pos < session->suspend_buffer_size && session->suspend_buffer[pos] == '-') {
            neg = true;
            ++pos;
        }
        for (; pos < session->suspend_buffer_size; ++pos) {
            char d = session->suspend_buffer[pos];
            if (d < '0' || d > '9')
                return raise_error(session, AkJSONErrorCode::INVALID_INTEGER_FORMAT);
            val = (val * 10) + (d - '0');
        }
        if (neg)
            val = -val;
        notify_value_number_int(session, val);
    } else {
        if (significant_digits > MAX_FLOAT_SIGNIFICANT_DIGITS) {
            return raise_error(session, AkJSONErrorCode::FLOAT_TOO_MANY_DIGITS);
        }
        // Parse float using locale-independent from_chars if available
        AkF64 v = 0.0;
        {
            const char *b = session->suspend_buffer;
            const char *e = session->suspend_buffer + session->suspend_buffer_size;
            std::from_chars_result r = std::from_chars(b, e, v, std::chars_format::general);
            if (r.ec != std::errc() || r.ptr != e) {
                return raise_error(session, AkJSONErrorCode::INVALID_FLOAT_FORMAT);
            }
        }
        notify_value_number_float(session, v);
    }
    session->suspend_buffer_size = 0;
    AK_MUST_TAIL return resume_parse_context(session, 0, head, end, json_size, 0);
}

static AkJSONParserState state_string_head(AkJSONParser *session, AkU32 sub_state, char *head, char *end, AkU64 json_size, AkU64 string_size) noexcept {
    bool is_single_buffer = (sub_state == 1);
    char* chunk_start = head;

    while (true) {
        if (head == end) {
            if (is_single_buffer) {
                if (chunk_start != head) {
                    notify_value_string_chunk(session, chunk_start, (AkU64)(head - chunk_start), 1);
                    string_size += (AkU64)(head - chunk_start);
                }
                is_single_buffer = false;
            } else {
                if (chunk_start != head) {
                    notify_value_string_chunk(session, chunk_start, (AkU64)(head - chunk_start), 1);
                    string_size += (AkU64)(head - chunk_start);
                }
            }
            return suspend_parser(session, state_string_head, 0, json_size, string_size);
        }
        char c = *head;
        if (c == '\\') {
            ++head; ++json_size;
            if (head == end) {
                // Escape splits across buffers. If we were still in single-buffer mode,
                // we need to switch to streaming and flush any pending chunk.
                if (is_single_buffer) {
                    if (chunk_start != (head - 1)) {
                        notify_value_string_chunk(session, chunk_start, (AkU64)((head - 1) - chunk_start), 1);
                        string_size += (AkU64)((head - 1) - chunk_start);
                    }
                    is_single_buffer = false;
                } else {
                    if (chunk_start != (head - 1)) {
                        notify_value_string_chunk(session, chunk_start, (AkU64)((head - 1) - chunk_start), 1);
                        string_size += (AkU64)((head - 1) - chunk_start);
                    }
                }
                // Emit the backslash as raw when escape spans buffers
                notify_value_string_chunk(session, "\\", 1, 1);
                ++string_size;
                chunk_start = head; // next chunk resumes after the backslash
                // No more chars in this buffer; suspend and continue in next buffer
                return suspend_parser(session, state_string_head, 0, json_size, string_size);
            }
            char e = *head; ++head; ++json_size;
            switch (e) {
                case '"': 
                case '\\': 
                case '/': 
                case 'b': 
                case 'f': 
                case 'n': 
                case 'r': 
                case 't':
                    break; // valid
                case 'u': {
                    AkU32 code1;
                    if (!parse_hex4(session, &head, end, &json_size, &code1)) {
                        if (session->state == AkJSONParserState::ERROR) return AkJSONParserState::ERROR;
                        return suspend_parser(session, state_string_head, 0, json_size, string_size);
                    }
                    if (code1 >= SURROGATE_HIGH_START && code1 <= SURROGATE_HIGH_END) {
                        if (head == end || *head != '\\') return raise_error(session, AkJSONErrorCode::INVALID_SURROGATE_PAIR);
                        ++head; ++json_size;
                        if (head == end || *head != 'u') return raise_error(session, AkJSONErrorCode::INVALID_SURROGATE_PAIR);
                        ++head; ++json_size;
                        AkU32 code2;
                        if (!parse_hex4(session, &head, end, &json_size, &code2)) {
                            if (session->state == AkJSONParserState::ERROR) return AkJSONParserState::ERROR;
                            return suspend_parser(session, state_string_head, 0, json_size, string_size);
                        }
                        if (!(code2 >= SURROGATE_LOW_START && code2 <= SURROGATE_LOW_END)) return raise_error(session, AkJSONErrorCode::INVALID_SURROGATE_PAIR);
                    } else if (code1 >= SURROGATE_LOW_START && code1 <= SURROGATE_LOW_END) {
                        return raise_error(session, AkJSONErrorCode::INVALID_SURROGATE_PAIR);
                    }
                    break;
                }
                default:
                    return raise_error(session, AkJSONErrorCode::INVALID_ESCAPE_CHAR);
            }
            continue;
        } else if (c == '"') {
            // end of string
            if (is_single_buffer) {
                notify_value_string(session, chunk_start, (AkU64)(head - chunk_start));
            } else {
                if (chunk_start != head) {
                    notify_value_string_chunk(session, chunk_start, (AkU64)(head - chunk_start), 1);
                    string_size += (AkU64)(head - chunk_start);
                }
                // Finalize chunked string
                notify_value_string_chunk(session, "", 0, 0);
            }
            ++head; ++json_size;
            // If we came here resuming from a suspend (streaming mode), the top
            // of the stack still holds the suspend frame for this string.
            // Drop it so that the following resume goes to the previous continuation
            // (e.g., return_state or list/object state).
            if (!is_single_buffer && session->stack_top > session->stack_begin) {
                session->stack_top--; // discard suspend frame for this string
            }
            AK_MUST_TAIL return resume_parse_context(session, 0, head, end, json_size, string_size);
        } else {
            ++head; ++json_size; continue;
        }
    }
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------------
// Notification Utilities - Unified Event System
// ------------------------------------------------------------------------------------------------------------------------------------------------------------

// Unified event notification function
static void notify_event(AkJSONParser *session, AkJSONEvent event_type, const AkJSONEventData *data, AkU64 more) noexcept {
    AK_ASSERT(session != nullptr);
    AK_ASSERT(session->on_event != nullptr);
    int rc = session->on_event(session, event_type, data, more);
    if (rc != 0) {
        (void)raise_error(session, AkJSONErrorCode::USER_ABORTED);
    }
}

// Parser state changed
static void notify_state_changed(AkJSONParser *session) noexcept {
    AkJSONEventData data = {};
    data.state_data.state = session->state;
    data.state_data.err_code = session->err_code;
    notify_event(session, AkJSONEvent::PARSE_STATE_CHANGED, &data);
}

// Object Notification
static void notify_object_begin(AkJSONParser *session) noexcept {
    notify_event(session, AkJSONEvent::OBJECT_BEGIN, nullptr, 0);
}

static void notify_object_end(AkJSONParser *session) noexcept {
    notify_event(session, AkJSONEvent::OBJECT_END, nullptr, 0);
}

static void notify_attr_key_chunk(AkJSONParser *session, const char *text_buffer, AkU64 text_buffer_length, AkU64 more) noexcept {
    AkJSONEventData data = {};
    data.string_data.str = text_buffer;
    data.string_data.len = text_buffer_length;
    notify_event(session, AkJSONEvent::ATTR_KEY, &data, more);
}

static void notify_attr_key(AkJSONParser *session, const char *text_buffer, AkU64 text_buffer_length) noexcept {
    AkJSONEventData data = {};
    data.string_data.str = text_buffer;
    data.string_data.len = text_buffer_length;
    notify_event(session, AkJSONEvent::ATTR_KEY, &data, 0);
}


// Literal Values
static void notify_value_null(AkJSONParser *session) noexcept {
    notify_event(session, AkJSONEvent::NULL_VALUE, nullptr, 0);
}

static void notify_value_bool(AkJSONParser *session, bool value) noexcept {
    AkJSONEventData data = {};
    data.bool_value = value;
    notify_event(session, AkJSONEvent::BOOL_VALUE, &data, 0);
}

static void notify_value_number_int(AkJSONParser *session, AkI64 value) noexcept {
    AkJSONEventData data = {};
    data.int_value = value;
    notify_event(session, AkJSONEvent::INT_VALUE, &data, 0);
}

static void notify_value_number_float(AkJSONParser *session, AkF64 value) noexcept {
    AkJSONEventData data = {};
    data.float_value = value;
    notify_event(session, AkJSONEvent::FLOAT_VALUE, &data, 0);
}

static void notify_value_string_chunk(AkJSONParser *session, const char *text_buffer, AkU64 text_buffer_length, AkU64 more) noexcept {
    AkJSONEventData data = {};
    data.string_data.str = text_buffer;
    data.string_data.len = text_buffer_length;
    notify_event(session, AkJSONEvent::STRING_VALUE, &data, more);
}

static void notify_value_string(AkJSONParser *session, const char *text_buffer, AkU64 text_buffer_length) noexcept {
    AkJSONEventData data = {};
    data.string_data.str = text_buffer;
    data.string_data.len = text_buffer_length;
    notify_event(session, AkJSONEvent::STRING_VALUE, &data, 0);
}

// Array  Notification
static void notify_array_begin(AkJSONParser *session) noexcept {
    notify_event(session, AkJSONEvent::ARRAY_BEGIN, nullptr, 0);
}

static void notify_array_end(AkJSONParser *session) noexcept {
    notify_event(session, AkJSONEvent::ARRAY_END, nullptr, 0);
}

// ------------------------------------------
// Utility implementations
// ------------------------------------------

static AkJSONParserState raise_error(AkJSONParser *session, AkJSONErrorCode code) noexcept {
    session->state = AkJSONParserState::ERROR;
    session->err_code = (AkU32)code;
    notify_state_changed(session);
    return AkJSONParserState::ERROR;
}

static inline bool push_parse_context_checked(AkJSONParser *session, AkJSONParserStateFn *fn, AkU32 sub_state) noexcept {
    if (!(session->stack_top < session->stack_end)) {
        (void)raise_error(session, AkJSONErrorCode::MAX_DEPTH_EXCEEDED);
        return false;
    }
    AkJSONParserCtx *ctx = session->stack_top;
    ctx->continuation = fn;
    ctx->sub_state = sub_state;
    ctx->user_data = nullptr;
    session->stack_top++;
    return true;
}

/// \internal Use push_parse_context_checked directly when pushing continuation frames
static AkJSONParserState resume_parse_context(AkJSONParser *session, AkU32 sub_state, char *head, char *end, AkU64 json_size, AkU64 string_size) noexcept {
    AK_ASSERT(session != nullptr);
    (void)sub_state;
    AK_ASSERT(session->stack_top > session->stack_begin);
    AkJSONParserCtx *top = session->stack_top - 1;
    AkJSONParserStateFn *continuation = top->continuation;
    AkU32 ret_sub = top->sub_state;
    session->stack_top = top; // pop one frame
    AK_MUST_TAIL return continuation(session, ret_sub, head, end, json_size, string_size);
}

static inline void emit_utf8_bytes(AkJSONParser *session, AkU32 cp, EmitFn *emit) {
    char bytes[4];
    AkU32 n = 0;
    if (cp < UTF8_1_MAX) {
        bytes[0] = (char)cp;
        n = 1;
    } else if (cp < UTF8_2_MAX) {
        bytes[0] = (char)(0xC0 | (cp >> 6));
        bytes[1] = (char)(0x80 | (cp & 0x3F));
        n = 2;
    } else if (cp < 0x10000U) {
        bytes[0] = (char)(0xE0 | (cp >> 12));
        bytes[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        bytes[2] = (char)(0x80 | (cp & 0x3F));
        n = 3;
    } else {
        bytes[0] = (char)(0xF0 | (cp >> 18));
        bytes[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
        bytes[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
        bytes[3] = (char)(0x80 | (cp & 0x3F));
        n = 4;
    }
    (*emit)(session, bytes, n);
}

static void emit_attr_key_utf8(AkJSONParser *session, const char *buf, AkU64 len) noexcept {
    notify_attr_key_chunk(session, buf, len, 1);
}

static inline CharClass classify_char(char c) noexcept {
    return static_cast<CharClass>(char_class_table[(AkU8)c]);
}

static inline bool is_number_char(char c) noexcept {
    return number_char_table[(AkU8)c] != 0;
}

static inline void prefetch_classification_tables() noexcept {
    __builtin_prefetch(&char_class_table[0], 0, 3);
    __builtin_prefetch(&number_char_table[0], 0, 3);
}

static inline char* skip_whitespace(char* head, char* end, AkU64* json_size) noexcept {
    while (head < end) {
        CharClass cls = classify_char(*head);
        if (cls != CHAR_WHITESPACE) {
            break;
        }
        ++head;
        ++(*json_size);
    }
    return head;
}
static inline bool parse_hex4(AkJSONParser *session, char **phead, char *end, AkU64 *pjson_size, AkU32 *pout) noexcept {
    char *head = *phead;
    AkU64 json_size = *pjson_size;
    AkU32 out = 0;
    for (int i = 0; i < 4; ++i) {
        if (head == end) {
            return false;
        }
        char h = *head;
        ++head;
        ++json_size;
        AkU32 d;
        if (h >= '0' && h <= '9') {
            d = (AkU32)(h - '0');
        } else if (h >= 'a' && h <= 'f') {
            d = 10u + (AkU32)(h - 'a');
        } else if (h >= 'A' && h <= 'F') {
            d = 10u + (AkU32)(h - 'A');
        } else {
            (void)raise_error(session, AkJSONErrorCode::INVALID_UNICODE_HEX_DIGIT);
            return false;
        }
        out = (out << 4) | d;
    }
    *phead = head;
    *pjson_size = json_size;
    *pout = out;
    return true;
}

static AkJSONParserState suspend_parser(AkJSONParser *session, AkJSONParserStateFn *fn, AkU32 sub_state, AkU64 json_size, AkU64 string_size) noexcept {
    // Push a suspend frame tagged via user_data to not clobber return continuations
    static int SUSP_TAG;
    if (!(session->stack_top < session->stack_end)) {
        return raise_error(session, AkJSONErrorCode::STACK_OVERFLOW_ON_SUSPEND);
    }
    AkJSONParserCtx *ctx = session->stack_top;
    ctx->continuation = fn;
    ctx->sub_state = sub_state;
    ctx->user_data = &SUSP_TAG;
    session->stack_top++;
    session->json_offset = json_size;
    session->string_offset = string_size;
    session->state = AkJSONParserState::CONTINUE;
    return AkJSONParserState::CONTINUE;
}
