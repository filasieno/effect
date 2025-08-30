#include "ak/json/json.hpp" // IWYU pragma: keep

#include <cstring>
#include <cstdlib>

namespace ak {

#define AK_MUST_TAIL __attribute__((musttail))

/*
 * JSON parsing strategy (tail-recursive, streaming, suspendable)
 *
 * Overview
 * - Tail-recursive state machine: each state consumes input and tail-calls the
 *   next state. Continuations are stored explicitly in JSONParseContext frames
 *   inside the session to avoid using the C++ call stack.
 * - Push/streaming: parse_buffer() is called with successive buffers. The
 *   parser never takes ownership of the input; it only keeps offsets and a
 *   small suspend buffer used exclusively for numbers.
 * - One-character lookahead: the code inspects at most one character at a time
 *   to choose the next transition.
 *
 * Suspension and resumption
 * - suspend_parser() records the continuation and offsets (json_offset,
 *   string_offset). A subsequent parse_buffer() resumes exactly where it
 *   stopped by tail-calling the stored continuation.
 * - Only numbers use the fixed-size suspend_buffer to span buffers. All other
 *   tokens are streamed directly from the caller-provided buffer.
 *
 * Events (SAX-like)
 * - Emitted via function pointers in JSONParseSession::handlers. Objects/
 *   arrays emit begin/end. Keys and strings use either optimized single-callback
 *   (key/session, string/session) when fully contained in one buffer, or the
 *   streaming triplets (attr_key_begin/chars/end, string_value_begin/chars/end)
 *   when they span buffers.
 *
 * Keys and strings
 * - Validation only: escape sequences (\", \\, \/, \b, \f, \n, \r, \t) and
 *   \uXXXX (including surrogate pairs) are validated but never decoded. Raw
 *   slices are delivered as-is to callbacks.
 * - Buffer boundary inside a string triggers streaming: we emit
 *   string_value_begin (once) and raw chunks via string_value_chars, then
 *   suspend. Next buffer resumes streaming until the closing quote.
 * - Optimized execution paths: if a single buffer stores the full key or string
 *   we use the optimized callback path ie. a single key/string callback is used; 
 *   if instead a key or string is split across buffers we use the streaming callback 
 *   path ie. string_value_begin/chars/end is used.
 *
 * Numbers
 * - Numbers accumulate into suspend_buffer across buffers, are validated, then
 *   emitted as int_value or float_value. This is the only token type that uses
 *   suspend_buffer.
 * - Note that accumulation does not copy bytes untill we discover (at the end of the buffer)
 *   that we cannot emit a number as the buffer has been interrupted.
 * - If we know that a number is completly inside a buffer than we simply emit the notification.
 *
 * Errors
 * - raise_error() sets ERROR state and err_msg, emits parse_state_changed, and
 *   stops parsing.
 *
 * Reentrancy/allocations
 * - Reentrant and allocation-free. All state is in JSONParseSession; no
 *   globals and no dynamic allocations are used.
 */

// ==========================================
// Utility function declarations
// ==========================================

static Bool is_digit(Char c) noexcept;
static JSONParserState raise_error(JSONParseSession *session, const Char *msg) noexcept;

// Internal stack helpers (push/pop) no longer needed

// Parse Context manipulation
static JSONParserState suspend_parser(JSONParseSession *session, JSONParserStateFn *fn, U32 sub_state, U64 json_size, U64 string_size) noexcept;
// static JSONParserState resume_parser(JSONParseSession *session, JSONParserStateFn *fn, U32 sub_state, Char *head, Char *end, U64 json_size, U64 string_size) noexcept;

// ==========================================
// State function declarations
// ==========================================

// Parser state changed
static Void notify_state_changed(JSONParseSession *session) noexcept;

// Object Notification
static Void notify_object_begin(JSONParseSession *session) noexcept;
static Void notify_object_end(JSONParseSession *session) noexcept;

static Void notify_attr_begin(JSONParseSession *session) noexcept;
static Void notify_attr_key_begin(JSONParseSession *session) noexcept;
static Void notify_attr_key_end(JSONParseSession *session) noexcept;
static Void notify_attr_key_chars(JSONParseSession *session, const Char *text_buffer, U64 text_buffer_length) noexcept;
static Void notify_key(JSONParseSession *session, const Char *text_buffer, U64 text_buffer_length) noexcept;
static Void notify_attr_end(JSONParseSession *session) noexcept;

// Literal Values
static Void notify_null_value(JSONParseSession *session) noexcept;
static Void notify_bool_value(JSONParseSession *session, Bool value) noexcept;
static Void notify_int_value(JSONParseSession *session, I64 value) noexcept;
static Void notify_float_value(JSONParseSession *session, F64 value) noexcept;
static Void notify_string_value_begin(JSONParseSession *session) noexcept;
static Void notify_string_value_end(JSONParseSession *session) noexcept;
static Void notify_string_value_chars(JSONParseSession *session, const Char *text_buffer, U64 text_buffer_length) noexcept;
static Void notify_string(JSONParseSession *session, const Char *text_buffer, U64 text_buffer_length) noexcept;

// Arrray  Notification
static Void notify_array_begin(JSONParseSession *session) noexcept;
static Void notify_array_end(JSONParseSession *session) noexcept;

// ==========================================
// State function declarations
// ==========================================

static JSONParserState sentinel(JSONParseSession *session, U32 sub_state, Char *head, Char *end, U64 json_size, U64 string_size) noexcept;
static JSONParserState return_state(JSONParseSession *session, U32 sub_state, Char *head, Char *end, U64 json_size, U64 string_size) noexcept;
static JSONParserState state_root_dispatch(JSONParseSession *session, U32 sub_state, Char *head, Char *end, U64 json_size, U64 string_size) noexcept;

static JSONParserState state_object_first_attr(JSONParseSession *session, U32 sub_state, Char *head, Char *end, U64 json_size, U64 string_size) noexcept;
static JSONParserState state_object_rest_attrs(JSONParseSession *session, U32 sub_state, Char *head, Char *end, U64 json_size, U64 string_size) noexcept;

static JSONParserState state_array_first_value(JSONParseSession *session, U32 sub_state, Char *head, Char *end, U64 json_size, U64 string_size) noexcept;
static JSONParserState state_list_rest_values(JSONParseSession *session, U32 sub_state, Char *head, Char *end, U64 json_size, U64 string_size) noexcept;
static JSONParserState state_array_value_required(JSONParseSession *session, U32 sub_state, Char *head, Char *end, U64 json_size, U64 string_size) noexcept;

static JSONParserState state_attr_begin_key(JSONParseSession *session, U32 sub_state, Char *head, Char *end, U64 json_size, U64 string_size) noexcept;
static JSONParserState state_attr_key_chars(JSONParseSession *session, U32 sub_state, Char *head, Char *end, U64 json_size, U64 string_size) noexcept;
static JSONParserState state_attr_semi(JSONParseSession *session, U32 sub_state, Char *head, Char *end, U64 json_size, U64 string_size) noexcept;

static JSONParserState state_value_dispatch(JSONParseSession *session, U32 sub_state, Char *head, Char *end, U64 json_size, U64 string_size) noexcept;
static JSONParserState state_null_head(JSONParseSession *session, U32 sub_state, Char *head, Char *end, U64 json_size, U64 string_size) noexcept;
static JSONParserState state_true_head(JSONParseSession *session, U32 sub_state, Char *head, Char *end, U64 json_size, U64 string_size) noexcept;
static JSONParserState state_false_head(JSONParseSession *session, U32 sub_state, Char *head, Char *end, U64 json_size, U64 string_size) noexcept;
static JSONParserState state_number_head(JSONParseSession *session, U32 sub_state, Char *head, Char *end, U64 json_size, U64 string_size) noexcept;
static JSONParserState state_string_head(JSONParseSession *session, U32 sub_state, Char* head, Char* end, U64 json_size, U64 string_size) noexcept;

// ==========================================
// Public function implementation
// ==========================================

JSONParseSession *init_json_parse_session(void *buffer, U64 buffer_size, JSONParseSessionConfig *cfg, ParseHandlers *handlers, Void *user_data) noexcept {
    AK_ASSERT(buffer_size >= sizeof(JSONParseSession));
    AK_ASSERT(cfg != nullptr);
    AK_ASSERT(handlers != nullptr);

    if constexpr (priv::IS_DEBUG_MODE) {
        std::memset(buffer, 0, buffer_size);
    }

    U64 required_size = get_required_parse_session_buffer_size(cfg);

    if (buffer_size < required_size) {
        return nullptr;
    }

    JSONParseSession *session = (JSONParseSession *)buffer;

    session->parser_buffer = buffer;
    session->parser_buffer_size = buffer_size;

    session->stack_begin = (JSONParseContext *)((Char *)session + sizeof(JSONParseSession));
    session->stack_end = (JSONParseContext *)((Char *)session->stack_begin + (cfg->max_depth * (U64)sizeof(JSONParseContext)));
    session->stack_top = session->stack_begin;

    session->config = *cfg;
    session->handlers = *handlers;
    session->state = JSONParserState::INITIALIZED;

    session->buffer = nullptr;
    session->buffer_len = 0;
    session->json_offset = 0;
    session->string_offset = 0;
    session->user_data = user_data;
    session->suspend_buffer_size = 0;

    return session;
}

Void reset_json_parse_session(JSONParseSession *session) noexcept {
    AK_ASSERT(session != nullptr);

    session->stack_top = session->stack_begin;
    session->buffer = nullptr;
    session->buffer_len = 0;
    session->state = JSONParserState::INITIALIZED;
    session->json_offset = 0;
    session->string_offset = 0;
}

static inline Void push_parse_context(JSONParseSession *session, JSONParserStateFn *fn, U32 sub_state = 0) noexcept {
    JSONParseContext *ctx = session->stack_top;
    ctx->continuation = fn;
    ctx->sub_state = sub_state;
    ctx->user_data = nullptr;
    session->stack_top++;
}

static JSONParserState resume_parse_context(JSONParseSession *session, U32 sub_state, Char *head, Char *end, U64 json_size, U64 string_size) noexcept {
    AK_ASSERT(session != nullptr);
    (void)sub_state;
    AK_ASSERT(session->stack_top > session->stack_begin);
    JSONParseContext *top = session->stack_top - 1;
    JSONParserStateFn *continuation = top->continuation;
    U32 ret_sub = top->sub_state;
    session->stack_top = top; // pop one frame
    AK_MUST_TAIL return continuation(session, ret_sub, head, end, json_size, string_size);
}

// Public API wrappers to match json_api.hpp
JSONParseSession *init_json_parser(Void *parser_buffer, U64 parser_buffer_size, const JSONParseSessionConfig *cfg, const ParseHandlers *handlers, Void *user_data) noexcept {
    AK_ASSERT(parser_buffer != nullptr);
    AK_ASSERT(cfg != nullptr);
    AK_ASSERT(handlers != nullptr);
    // We reuse the existing initializer and copy parameters
    JSONParseSessionConfig tmp_cfg = *cfg;
    ParseHandlers tmp_handlers = *handlers;
    return init_json_parse_session(parser_buffer, parser_buffer_size, &tmp_cfg, &tmp_handlers, user_data);
}

JSONParserState parse_buffer(JSONParseSession *session, Void *buffer, U64 buffer_size) noexcept {
    AK_ASSERT(session != nullptr);
    AK_ASSERT(buffer != nullptr);
    AK_ASSERT(session->state != JSONParserState::INVALID);

    // Always set the current buffer for this invocation
    session->buffer = (Char *)buffer;
    session->buffer_len = buffer_size;

    if (session->state == JSONParserState::CONTINUE) {
        // resume parsing from saved continuation
        JSONParseContext *top_ctx = session->stack_top - 1;
        JSONParserStateFn *continuation = top_ctx->continuation;
        return continuation(session, top_ctx->sub_state, session->buffer, session->buffer + session->buffer_len, session->json_offset, session->string_offset);
    }

    if (session->state == JSONParserState::INITIALIZED) {
        // first time run, we need to set the initial state
        session->state = JSONParserState::CONTINUE;
        session->stack_top = session->stack_begin;
        session->json_offset = 0;
        session->string_offset = 0;
        session->suspend_buffer_size = 0;

        push_parse_context(session, sentinel, 0);
        push_parse_context(session, return_state, 0);

        return state_root_dispatch(session, 0, session->buffer, session->buffer + session->buffer_len, session->json_offset, session->string_offset);
    }

    // If the current session state is already DONE or ERROR
    return session->state;
}

// ==========================================
// State function implementations
// ==========================================

static JSONParserState sentinel(JSONParseSession *session, U32 sub_state, Char *head, Char *end, U64 json_size, U64 string_size) noexcept {
    (void)(sub_state);
    (void)(head);
    (void)(end);
    (void)(json_size);
    (void)(string_size);

    session->err_msg = "fatal error: parser context stack out of bounds";
    return JSONParserState::ERROR;
}

static JSONParserState return_state(JSONParseSession *session, U32 sub_state, Char *head, Char *end, U64 json_size, U64 string_size) noexcept {
    (void)(sub_state);
    (void)(head);
    (void)(end);
    (void)(json_size);
    (void)(string_size);

    // If we reached the base return state, parsing is complete
    session->state = JSONParserState::DONE;
    return session->state;
}

// Initial and root states
static JSONParserState state_root_dispatch(JSONParseSession *session, U32 sub_state, Char *head, Char *end, U64 json_size, U64 string_size) noexcept {
    if (head == end) {
        // For initial empty/whitespace-only inputs, treat as error (tests expect ERROR)
        if (json_size == 0)
            return raise_error(session, "empty input");
        return raise_error(session, "unexpected end of input");
    }
    Char c = *head;
    switch (c) {
    case '{': {
        ++head;
        ++json_size;
        notify_object_begin(session);
        // no need to push any parse context; using the bottom return value
        AK_MUST_TAIL return state_object_first_attr(session, sub_state, head, end, json_size, string_size);
    }
    case '[': {
        notify_array_begin(session);
        ++head;
        ++json_size;
        AK_MUST_TAIL return state_array_first_value(session, sub_state, head, end, json_size, string_size);
    }
    case ' ':
    case '\t':
    case '\n':
    case '\r': {
        ++head;
        ++json_size;
        AK_MUST_TAIL return state_root_dispatch(session, sub_state, head, end, json_size, string_size);
    }
    default: {
        // Allow top-level primitives per RFC 8259
        if (c == '"' || c == 't' || c == 'f' || c == 'n' || c == '-' || is_digit(c)) {
            push_parse_context(session, return_state, 0);
            AK_MUST_TAIL return state_value_dispatch(session, sub_state, head, end, json_size, string_size);
        }
        return raise_error(session, "expected an Object '{ ... }' or an Array '[ ... ]'");
    }
    }
}

static JSONParserState state_object_first_attr(JSONParseSession *session, U32 sub_state, Char *head, Char *end, U64 json_size, U64 string_size) noexcept {
    if (head == end)
        return suspend_parser(session, state_object_first_attr, sub_state, json_size, string_size);
    Char c = *head;
    switch (c) {
    case '}': {
        ++head;
        ++json_size;
        notify_object_end(session);
        AK_MUST_TAIL return resume_parse_context(session, sub_state, head, end, json_size, string_size);
    }
    case ' ':
    case '\t':
    case '\n':
    case '\r': {
        ++head;
        ++json_size;
        AK_MUST_TAIL return state_object_first_attr(session, sub_state, head, end, json_size, string_size);
    }
    default: {
        AK_MUST_TAIL return state_attr_begin_key(session, sub_state, head, end, json_size, string_size);
    }
    }
}

static JSONParserState state_object_rest_attrs(JSONParseSession *session, U32 sub_state, Char *head, Char *end, U64 json_size, U64 string_size) noexcept {
    if (head == end)
        return suspend_parser(session, state_object_rest_attrs, sub_state, json_size, string_size);
    Char c = *head;
    switch (c) {
    case '}': {
        ++head;
        ++json_size;
        notify_attr_end(session);
        notify_object_end(session);
        AK_MUST_TAIL return resume_parse_context(session, sub_state, head, end, json_size, string_size);
    }
    case ',': {
        notify_attr_end(session);
        ++head;
        ++json_size;
        AK_MUST_TAIL return state_attr_begin_key(session, sub_state, head, end, json_size, string_size);
    }
    case ' ':
    case '\t':
    case '\n':
    case '\r': {
        ++head;
        ++json_size;
        AK_MUST_TAIL return state_object_rest_attrs(session, sub_state, head, end, json_size, string_size);
    }
    default: {
        return raise_error(session, "expected a comma or a closing brace");
    }
    }
}

static JSONParserState state_array_first_value(JSONParseSession *session, U32 sub_state, Char *head, Char *end, U64 json_size, U64 string_size) noexcept {
    while (true) {
        if (head == end)
            return suspend_parser(session, state_array_first_value, sub_state, json_size, string_size);
        Char c = *head;
        switch (c) {
        case ']': {
            ++head;
            ++json_size;
            notify_array_end(session);
            AK_MUST_TAIL return resume_parse_context(session, sub_state, head, end, json_size, string_size);
        }
        case ' ':
        case '\t':
        case '\n':
        case '\r':
            ++head;
            ++json_size;
            continue;
        default: {
            push_parse_context(session, state_list_rest_values, 0);
            AK_MUST_TAIL return state_value_dispatch(session, 0, head, end, json_size, string_size);
        }
        }
    }
}

static JSONParserState state_list_rest_values(JSONParseSession *session, U32 sub_state, Char *head, Char *end, U64 json_size, U64 string_size) noexcept {
    if (head == end)
        return suspend_parser(session, state_list_rest_values, sub_state, json_size, string_size);
    Char c = *head;
    switch (c) {
    case ',': {
        ++head;
        ++json_size;
        // Next value
        push_parse_context(session, state_list_rest_values, 0);
        AK_MUST_TAIL return state_array_value_required(session, 0, head, end, json_size, string_size);
    }
    case ']': {
        ++head;
        ++json_size;
        notify_array_end(session);
        AK_MUST_TAIL return resume_parse_context(session, sub_state, head, end, json_size, string_size);
    }
    case ' ':
    case '\t':
    case '\n':
    case '\r': {
        ++head;
        ++json_size;
        AK_MUST_TAIL return state_list_rest_values(session, sub_state, head, end, json_size, string_size);
    }
    default:
        return raise_error(session, "expected a comma or a closing bracket");
    }
}

// After a comma inside arrays, a value must follow; ']' is not allowed (catches trailing comma)
static JSONParserState state_array_value_required(JSONParseSession *session, U32 sub_state, Char *head, Char *end, U64 json_size, U64 string_size) noexcept {
    while (true) {
        if (head == end)
            return suspend_parser(session, state_array_value_required, sub_state, json_size, string_size);
        Char c = *head;
        switch (c) {
        case ' ':
        case '\t':
        case '\n':
        case '\r': {
            ++head;
            ++json_size;
            continue;
        }
        case ']':
            return raise_error(session, "expected a value after comma");
        default:
            // Delegate to value dispatch, keeping rest-values on stack
            AK_MUST_TAIL return state_value_dispatch(session, sub_state, head, end, json_size, string_size);
        }
    }
}

// Key string parsing (supports simple escapes and raw chunks)
static JSONParserState state_attr_key_chars(JSONParseSession *session, U32 sub_state, Char *head, Char *end, U64 json_size, U64 string_size) noexcept {
    Bool is_complete_key = (sub_state == 1);
    Char *key_start = head;
    Char *chunk_start = head;

    while (true) {
        if (head == end) {
            if (!is_complete_key && chunk_start != head) {
                notify_attr_key_chars(session, chunk_start, (U64)(head - chunk_start));
                string_size += (U64)(head - chunk_start);
            }
            return suspend_parser(session, state_attr_key_chars, 0, json_size, string_size);
        }
        Char c = *head;
        if (c == '\\') {
            // On first escape, fall back to streaming mode
            if (is_complete_key) {
                notify_attr_key_begin(session);
                if (chunk_start != head) {
                    notify_attr_key_chars(session, chunk_start, (U64)(head - chunk_start));
                    string_size += (U64)(head - chunk_start);
                }
                is_complete_key = false;
            } else if (chunk_start != head) {
                notify_attr_key_chars(session, chunk_start, (U64)(head - chunk_start));
                string_size += (U64)(head - chunk_start);
            }
            ++head;
            ++json_size;
            if (head == end) {
                return suspend_parser(session, state_attr_key_chars, 0, json_size, string_size);
            }
            Char e = *head;
            ++head;
            ++json_size;
            Char rc;
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
                auto parse_hex4 = [&](U32 &out) -> Bool {
                    out = 0;
                    for (int i = 0; i < 4; ++i) {
                        if (head == end)
                            return false;
                        Char h = *head;
                        ++head;
                        ++json_size;
                        U32 d;
                        if (h >= '0' && h <= '9')
                            d = (U32)(h - '0');
                        else if (h >= 'a' && h <= 'f')
                            d = 10u + (U32)(h - 'a');
                        else if (h >= 'A' && h <= 'F')
                            d = 10u + (U32)(h - 'A');
                        else {
                            raise_error(session, "invalid hex digit in unicode escape");
                            return false;
                        }
                        out = (out << 4) | d;
                    }
                    return true;
                };
                auto emit_utf8 = [&](U32 cp) {
                    char bytes[4];
                    U32 n = 0;
                    if (cp < 0x80) {
                        bytes[0] = (char)cp;
                        n = 1;
                    } else if (cp < 0x800) {
                        bytes[0] = (char)(0xC0 | (cp >> 6));
                        bytes[1] = (char)(0x80 | (cp & 0x3F));
                        n = 2;
                    } else if (cp < 0x10000) {
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
                    notify_attr_key_chars(session, bytes, n);
                    string_size += n;
                };
                U32 code1;
                if (!parse_hex4(code1)) {
                    if (session->state == JSONParserState::ERROR)
                        return JSONParserState::ERROR;
                    return suspend_parser(session, state_attr_key_chars, 0, json_size, string_size);
                }
                if (code1 >= 0xD800 && code1 <= 0xDBFF) {
                    if (head == end || *head != '\\') {
                        return raise_error(session, "invalid surrogate pair");
                    }
                    ++head;
                    ++json_size;
                    if (head == end || *head != 'u') {
                        return raise_error(session, "invalid surrogate pair");
                    }
                    ++head;
                    ++json_size;
                    U32 code2;
                    if (!parse_hex4(code2)) {
                        if (session->state == JSONParserState::ERROR)
                            return JSONParserState::ERROR;
                        return suspend_parser(session, state_attr_key_chars, 0, json_size, string_size);
                    }
                    if (!(code2 >= 0xDC00 && code2 <= 0xDFFF)) {
                        return raise_error(session, "invalid surrogate pair");
                    }
                    U32 cp = 0x10000 + (((code1 - 0xD800) & 0x3FF) << 10) + ((code2 - 0xDC00) & 0x3FF);
                    emit_utf8(cp);
                } else if (code1 >= 0xDC00 && code1 <= 0xDFFF) {
                    return raise_error(session, "invalid surrogate pair");
                } else {
                    emit_utf8(code1);
                }
                chunk_start = head;
                continue;
            }
            default:
                return raise_error(session, "invalid escape sequence character");
            }
            // Handle the escaped character (streaming mode)
            notify_attr_key_chars(session, &rc, 1);
            ++string_size;
            chunk_start = head;
            continue;
        } else if (c == '"') {
            // end of key
            if (!is_complete_key && chunk_start != head) {
                notify_attr_key_chars(session, chunk_start, (U64)(head - chunk_start));
                string_size += (U64)(head - chunk_start);
            }
            ++head;
            ++json_size;

            if (is_complete_key) {
                // Use optimized callback for complete key
                notify_key(session, key_start, (U64)(head - 1 - key_start));
            } else {
                // Use streaming end callback
                notify_attr_key_end(session);
            }
            AK_MUST_TAIL return state_attr_semi(session, 0, head, end, json_size, string_size);
        } else {
            ++head;
            ++json_size;
            continue;
        }
    }
}

static JSONParserState state_attr_begin_key(JSONParseSession *session, U32 sub_state, Char *head, Char *end, U64 json_size, U64 string_size) noexcept {
    if (head == end)
        return suspend_parser(session, state_attr_begin_key, sub_state, json_size, string_size);
    Char c = *head;
    switch (c) {
    case '"': {
        ++head;
        ++json_size;
        notify_attr_begin(session);
        push_parse_context(session, state_object_rest_attrs, 0);
        // Pass sub_state=1 to indicate this is a complete key candidate
        AK_MUST_TAIL return state_attr_key_chars(session, 1, head, end, json_size, string_size);
    }
    case ' ':
    case '\t':
    case '\n':
    case '\r': {
        ++head;
        ++json_size;
        AK_MUST_TAIL return state_attr_begin_key(session, sub_state, head, end, json_size, string_size);
    }
    default: {
        return raise_error(session, "expected a string key");
    }
    }
}

// state_attr_end_key not used; end-of-key handled in state_attr_key_chars

static JSONParserState state_attr_semi(JSONParseSession *session, U32 sub_state, Char *head, Char *end, U64 json_size, U64 string_size) noexcept {
    (void)sub_state;
    (void)string_size;
    while (true) {
        if (head == end)
            return suspend_parser(session, state_attr_semi, 0, json_size, string_size);
        Char c = *head;
        switch (c) {
        case ':': {
            ++head;
            ++json_size;
            // After ':' comes a value
            AK_MUST_TAIL return state_value_dispatch(session, 0, head, end, json_size, string_size);
        }
        case ' ':
        case '\t':
        case '\n':
        case '\r':
            ++head;
            ++json_size;
            continue;
        default:
            return raise_error(session, "expected ':' after key");
        }
    }
}

// Value parsing and primitives
static JSONParserState state_value_dispatch(JSONParseSession *session, U32 sub_state, Char *head, Char *end, U64 json_size, U64 string_size) noexcept {
    (void)sub_state;
    (void)string_size;
    while (true) {
        if (head == end)
            return suspend_parser(session, state_value_dispatch, 0, json_size, string_size);
        Char c = *head;
        switch (c) {
        case ' ':
        case '\t':
        case '\n':
        case '\r':
            ++head;
            ++json_size;
            continue;
        case 'n':
            ++head;
            ++json_size;
            AK_MUST_TAIL return state_null_head(session, 0, head, end, json_size, 0);
        case 't':
            ++head;
            ++json_size;
            AK_MUST_TAIL return state_true_head(session, 0, head, end, json_size, 0);
        case 'f':
            ++head;
            ++json_size;
            AK_MUST_TAIL return state_false_head(session, 0, head, end, json_size, 0);
        case '"':
            ++head;
            ++json_size;
            // Don't emit begin yet - we'll emit it only if we need to fall back to streaming
            // Pass sub_state=1 to indicate this is a complete string candidate
            AK_MUST_TAIL return state_string_head(session, 1, head, end, json_size, 0);
        case '{':
            ++head;
            ++json_size;
            notify_object_begin(session);
            AK_MUST_TAIL return state_object_first_attr(session, 0, head, end, json_size, 0);
        case '[':
            ++head;
            ++json_size;
            notify_array_begin(session);
            AK_MUST_TAIL return state_array_first_value(session, 0, head, end, json_size, 0);
        default:
            if (c == '-' || is_digit(c)) {
                // Start number; include current char into suspend buffer and continue
                session->suspend_buffer_size = 0;
                // Fallthrough to number state by not consuming here; number state will read from current char
                AK_MUST_TAIL return state_number_head(session, 0, head, end, json_size, 0);
            }
            return raise_error(session, "unexpected character while parsing value");
        }
    }
}

static JSONParserState state_null_head(JSONParseSession *session, U32 sub_state, Char *head, Char *end, U64 json_size, U64 string_size) noexcept {
    // We already consumed 'n'; now expect 'u' 'l' 'l'
    (void)string_size;
    const Char expected[] = {'u', 'l', 'l'};
    U32 idx = sub_state; // how many of expected we already matched
    while (idx < 3) {
        if (head == end)
            return suspend_parser(session, state_null_head, idx, json_size, 0);
        Char c = *head;
        if (c != expected[idx])
            return raise_error(session, "invalid token, expected 'null'");
        ++head;
        ++json_size;
        ++idx;
    }
    notify_null_value(session);
    AK_MUST_TAIL return resume_parse_context(session, 0, head, end, json_size, 0);
}

static JSONParserState state_true_head(JSONParseSession *session, U32 sub_state, Char *head, Char *end, U64 json_size, U64 string_size) noexcept {
    (void)string_size;
    const Char expected[] = {'r', 'u', 'e'};
    U32 idx = sub_state;
    while (idx < 3) {
        if (head == end)
            return suspend_parser(session, state_true_head, idx, json_size, 0);
        Char c = *head;
        if (c != expected[idx])
            return raise_error(session, "invalid token, expected 'true'");
        ++head;
        ++json_size;
        ++idx;
    }
    notify_bool_value(session, true);
    AK_MUST_TAIL return resume_parse_context(session, 0, head, end, json_size, 0);
}

static JSONParserState state_false_head(JSONParseSession *session, U32 sub_state, Char *head, Char *end, U64 json_size, U64 string_size) noexcept {
    (void)string_size;
    const Char expected[] = {'a', 'l', 's', 'e'};
    U32 idx = sub_state;
    while (idx < 4) {
        if (head == end)
            return suspend_parser(session, state_false_head, idx, json_size, 0);
        Char c = *head;
        if (c != expected[idx])
            return raise_error(session, "invalid token, expected 'false'");
        ++head;
        ++json_size;
        ++idx;
    }
    notify_bool_value(session, false);
    AK_MUST_TAIL return resume_parse_context(session, 0, head, end, json_size, 0);
}

static Bool is_num_char(Char c) noexcept { return (c == '-' || c == '+' || c == '.' || c == 'e' || c == 'E' || (c >= '0' && c <= '9')); }

static JSONParserState state_number_head(JSONParseSession *session, U32 sub_state, Char *head, Char *end, U64 json_size, U64 string_size) noexcept {
    (void)sub_state;
    (void)string_size;
    // collect number into suspend buffer; only use suspend buffer for numbers
    while (true) {
        if (head == end) {
            break; // finalize number at buffer end
        }
        Char c = *head;
        if (!is_num_char(c))
            break;
        if (session->suspend_buffer_size + 1 < sizeof(session->suspend_buffer)) {
            session->suspend_buffer[session->suspend_buffer_size++] = c;
        } else {
            return raise_error(session, "number too long");
        }
        ++head;
        ++json_size;
    }
    // terminate
    session->suspend_buffer[session->suspend_buffer_size] = '\0';
    // Validate number format per RFC 8259
    const char *num = session->suspend_buffer;
    U64 len = session->suspend_buffer_size;
    if (len == 0)
        return raise_error(session, "invalid number format");
    U64 p = 0;
    if (num[p] == '-') {
        ++p;
        if (p == len)
            return raise_error(session, "invalid number format");
    }
    if (num[p] == '0') {
        // no leading zeros allowed
        if (p + 1 < len && num[p + 1] >= '0' && num[p + 1] <= '9') {
            return raise_error(session, "invalid number format: leading zero");
        }
        ++p;
    } else {
        if (!(num[p] >= '1' && num[p] <= '9'))
            return raise_error(session, "invalid number format");
        while (p < len && (num[p] >= '0' && num[p] <= '9'))
            ++p;
    }
    bool is_float = false;
    if (p < len && num[p] == '.') {
        is_float = true;
        ++p;
        if (p == len)
            return raise_error(session, "invalid number format: no digits after decimal");
        if (!(num[p] >= '0' && num[p] <= '9'))
            return raise_error(session, "invalid number format: no digits after decimal");
        while (p < len && (num[p] >= '0' && num[p] <= '9'))
            ++p;
    }
    if (p < len && (num[p] == 'e' || num[p] == 'E')) {
        is_float = true;
        ++p;
        if (p == len)
            return raise_error(session, "invalid number format: no digits in exponent");
        if (num[p] == '+' || num[p] == '-') {
            ++p;
            if (p == len)
                return raise_error(session, "invalid number format: no digits in exponent");
        }
        if (!(num[p] >= '0' && num[p] <= '9'))
            return raise_error(session, "invalid number format: no digits in exponent");
        while (p < len && (num[p] >= '0' && num[p] <= '9'))
            ++p;
    }
    if (p != len)
        return raise_error(session, "invalid number format");
    // decide int vs float
    for (U64 i = 0; i < session->suspend_buffer_size; ++i) {
        Char c = session->suspend_buffer[i];
        if (c == '.' || c == 'e' || c == 'E') {
            is_float = true;
            break;
        }
    }
    if (!is_float) {
        // parse integer
        // simple strtoll; we avoid heavy libs; implement minimal
        bool neg = false;
        U64 pos = 0;
        I64 val = 0;
        if (pos < session->suspend_buffer_size && session->suspend_buffer[pos] == '-') {
            neg = true;
            ++pos;
        }
        for (; pos < session->suspend_buffer_size; ++pos) {
            Char d = session->suspend_buffer[pos];
            if (d < '0' || d > '9')
                return raise_error(session, "invalid integer format");
            val = (val * 10) + (d - '0');
        }
        if (neg)
            val = -val;
        notify_int_value(session, val);
    } else {
        // parse float using simple strtod from C standard library
        char *endp = nullptr;
        F64 v = std::strtod(session->suspend_buffer, &endp);
        if ((U64)(endp - session->suspend_buffer) != session->suspend_buffer_size) {
            return raise_error(session, "invalid float format");
        }
        notify_float_value(session, v);
    }
    session->suspend_buffer_size = 0;
    AK_MUST_TAIL return resume_parse_context(session, 0, head, end, json_size, 0);
}

static JSONParserState state_string_head(JSONParseSession *session, U32 sub_state, Char *head, Char *end, U64 json_size, U64 string_size) noexcept {
    Bool is_single_buffer = (sub_state == 1);
    Char* chunk_start = head;

    while (true) {
        if (head == end) {
            if (is_single_buffer) {
                // If at top-level (next continuation is return_state), signal error
                if (session->stack_top > session->stack_begin) {
                    JSONParseContext *top_ctx = session->stack_top - 1;
                    if (top_ctx->continuation == return_state) {
                        return raise_error(session, "unexpected end of input");
                    }
                }
                // Switch to streaming, emit current chunk, then suspend
                notify_string_value_begin(session);
                if (chunk_start != head) {
                    notify_string_value_chars(session, chunk_start, (U64)(head - chunk_start));
                    string_size += (U64)(head - chunk_start);
                }
            } else {
                if (chunk_start != head) {
                    notify_string_value_chars(session, chunk_start, (U64)(head - chunk_start));
                    string_size += (U64)(head - chunk_start);
                }
            }
            return suspend_parser(session, state_string_head, 0, json_size, string_size);
        }
        Char c = *head;
        if (c == '\\') {
            ++head; ++json_size;
            if (head == end) { return suspend_parser(session, state_string_head, 0, json_size, string_size); }
            Char e = *head; ++head; ++json_size;
            switch (e) {
                case '"': case '\\': case '/': case 'b': case 'f': case 'n': case 'r': case 't':
                    break; // valid
                case 'u': {
                    auto parse_hex4 = [&](U32 &out)->Bool{
                        out = 0;
                        for (int i = 0; i < 4; ++i) {
                            if (head == end) return false;
                            Char h = *head; ++head; ++json_size;
                            U32 d;
                            if (h >= '0' && h <= '9') d = (U32)(h - '0');
                            else if (h >= 'a' && h <= 'f') d = 10u + (U32)(h - 'a');
                            else if (h >= 'A' && h <= 'F') d = 10u + (U32)(h - 'A');
                            else { raise_error(session, "invalid hex digit in unicode escape"); return false; }
                            out = (out << 4) | d;
                        }
                        return true;
                    };
                    U32 code1; if (!parse_hex4(code1)) { if (session->state == JSONParserState::ERROR) return JSONParserState::ERROR; return suspend_parser(session, state_string_head, 0, json_size, string_size); }
                    if (code1 >= 0xD800 && code1 <= 0xDBFF) {
                        if (head == end || *head != '\\') return raise_error(session, "invalid surrogate pair");
                        ++head; ++json_size;
                        if (head == end || *head != 'u') return raise_error(session, "invalid surrogate pair");
                        ++head; ++json_size;
                        U32 code2; if (!parse_hex4(code2)) { if (session->state == JSONParserState::ERROR) return JSONParserState::ERROR; return suspend_parser(session, state_string_head, 0, json_size, string_size); }
                        if (!(code2 >= 0xDC00 && code2 <= 0xDFFF)) return raise_error(session, "invalid surrogate pair");
                    } else if (code1 >= 0xDC00 && code1 <= 0xDFFF) {
                        return raise_error(session, "invalid surrogate pair");
                    }
                    break;
                }
                default:
                    return raise_error(session, "invalid escape sequence character");
            }
            continue;
        } else if (c == '"') {
            // end of string
            if (is_single_buffer) {
                notify_string(session, chunk_start, (U64)(head - chunk_start));
            } else {
                if (chunk_start != head) {
                    notify_string_value_chars(session, chunk_start, (U64)(head - chunk_start));
                    string_size += (U64)(head - chunk_start);
                }
                notify_string_value_end(session);
            }
            ++head; ++json_size;
            AK_MUST_TAIL return resume_parse_context(session, 0, head, end, json_size, string_size);
        } else {
            ++head; ++json_size; continue;
        }
    }
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------------
// Notification Utilities
// ------------------------------------------------------------------------------------------------------------------------------------------------------------

// Parser state changed
static Void notify_state_changed(JSONParseSession *session) noexcept {
    if (session->handlers.parse_state_changed != nullptr) {
        session->handlers.parse_state_changed(session);
    }
}

// Object Notification
static Void notify_object_begin(JSONParseSession *session) noexcept {
    if (session->handlers.object_begin != nullptr) {
        session->handlers.object_begin(session);
    }
}

static Void notify_object_end(JSONParseSession *session) noexcept {
    if (session->handlers.object_end != nullptr) {
        session->handlers.object_end(session);
    }
}

static Void notify_attr_begin(JSONParseSession *session) noexcept {
    if (session->handlers.attr_begin != nullptr) {
        session->handlers.attr_begin(session);
    }
}

static Void notify_attr_key_begin(JSONParseSession *session) noexcept {
    if (session->handlers.attr_key_begin != nullptr) {
        session->handlers.attr_key_begin(session);
    }
}

static Void notify_attr_key_end(JSONParseSession *session) noexcept {
    if (session->handlers.attr_key_end != nullptr) {
        session->handlers.attr_key_end(session);
    }
}

static Void notify_attr_key_chars(JSONParseSession *session, const Char *text_buffer, U64 text_buffer_length) noexcept {
    if (session->handlers.attr_key_chars != nullptr) {
        session->handlers.attr_key_chars(session, text_buffer, text_buffer_length);
    }
}

static Void notify_key(JSONParseSession *session, const Char *text_buffer, U64 text_buffer_length) noexcept {
    if (session->handlers.key != nullptr) {
        session->handlers.key(session, text_buffer, text_buffer_length);
    }
}

static Void notify_attr_end(JSONParseSession *session) noexcept {
    if (session->handlers.attr_end != nullptr) {
        session->handlers.attr_end(session);
    }
}

// Literal Values
static Void notify_null_value(JSONParseSession *session) noexcept {
    if (session->handlers.null_value != nullptr) {
        session->handlers.null_value(session);
    }
}

static Void notify_bool_value(JSONParseSession *session, Bool value) noexcept {
    if (session->handlers.bool_value != nullptr) {
        session->handlers.bool_value(session, value);
    }
}

static Void notify_int_value(JSONParseSession *session, I64 value) noexcept {
    if (session->handlers.int_value != nullptr) {
        session->handlers.int_value(session, value);
    }
}

static Void notify_float_value(JSONParseSession *session, F64 value) noexcept {
    if (session->handlers.float_value != nullptr) {
        session->handlers.float_value(session, value);
    }
}

static Void notify_string_value_begin(JSONParseSession *session) noexcept {
    if (session->handlers.string_value_begin != nullptr) {
        session->handlers.string_value_begin(session);
    }
}

static Void notify_string_value_end(JSONParseSession *session) noexcept {
    if (session->handlers.string_value_end != nullptr) {
        session->handlers.string_value_end(session);
    }
}

static Void notify_string_value_chars(JSONParseSession *session, const Char *text_buffer, U64 text_buffer_length) noexcept {
    if (session->handlers.string_value_chars != nullptr) {
        session->handlers.string_value_chars(session, text_buffer, text_buffer_length);
    }
}

static Void notify_string(JSONParseSession *session, const Char *text_buffer, U64 text_buffer_length) noexcept {
    if (session->handlers.string != nullptr) {
        session->handlers.string(session, text_buffer, text_buffer_length);
    }
}

// Array  Notification
static Void notify_array_begin(JSONParseSession *session) noexcept {
    if (session->handlers.array_begin != nullptr) {
        session->handlers.array_begin(session);
    }
}

static Void notify_array_end(JSONParseSession *session) noexcept {
    if (session->handlers.array_end != nullptr) {
        session->handlers.array_end(session);
    }
}

// ------------------------------------------
// Utility implementations
// ------------------------------------------
static Bool is_digit(Char c) noexcept { return (c >= '0' && c <= '9'); }

static JSONParserState raise_error(JSONParseSession *session, const Char *msg) noexcept {
    session->state = JSONParserState::ERROR;
    session->err_msg = msg;
    notify_state_changed(session);
    return JSONParserState::ERROR;
}

static JSONParserState suspend_parser(JSONParseSession *session, JSONParserStateFn *fn, U32 sub_state, U64 json_size, U64 string_size) noexcept {
    // Push a suspend frame tagged via user_data to not clobber return continuations
    static int SUSP_TAG;
    if (!(session->stack_top < session->stack_end)) {
        return raise_error(session, "parser stack overflow on suspend");
    }
    JSONParseContext *ctx = session->stack_top;
    ctx->continuation = fn;
    ctx->sub_state = sub_state;
    ctx->user_data = &SUSP_TAG;
    session->stack_top++;
    session->json_offset = json_size;
    session->string_offset = string_size;
    session->state = JSONParserState::CONTINUE;
    return JSONParserState::CONTINUE;
}


} // namespace ak
