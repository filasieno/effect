# SAX-like JSON Parser Specification

## Overview

This specification describes a SAX (Simple API for XML)-like parser for JSON data. Unlike traditional parsers that build a complete data structure in memory, a SAX-like parser processes the input sequentially and invokes user-provided callback functions (handlers) for each significant event encountered in the JSON stream, such as the start of an object, a key-value pair, or a primitive value.

### Key Features

1. **Resumable**: The parser can handle partial input and resume parsing when more data becomes available. This is useful for streaming or network-based inputs.
2. **Tail Recursive Routines**: The parser is implemented using tail-recursive functions to model state transitions. These are optimized by Clang to avoid stack overflow and improve performance.
3. **State in Arguments**: All parsing state is passed as function arguments to encourage the compiler to keep them in registers, minimizing memory access and improving speed. No local variables are used in the parsing functions.
4. **Event-Driven**: Users provide a set of handler functions that are called at specific points during parsing.

## Data Structures

### JSONParserState

An enumeration representing the current state of the parser.

```C++
enum class JSONParserState {
    INVALID,      // Invalid state, e.g., after an error
    INITIALIZED,  // Parser is ready to start
    CONTINUE,     // Parsing can continue with more data
    DONE,         // Parsing completed successfully
    ERROR         // An error occurred during parsing
};
```

### ParseHandlers

A structure containing user-provided callback functions. Each callback is invoked when a specific JSON event is encountered. All callbacks take a `void* user_data` for custom user context.

```C++
struct ParseHandlers {
    void (*start_object)(void* user_data);                  // Called at the start of a JSON object '{'
    void (*end_object)(void* user_data);                    // Called at the end of a JSON object '}'
    void (*start_array)(void* user_data);                   // Called at the start of a JSON array '['
    void (*end_array)(void* user_data);                     // Called at the end of a JSON array ']'
    void (*key)(void* user_data, const char* str, size_t len); // Called for a JSON object key (string)
    void (*string)(void* user_data, const char* str, size_t len); // Called for a JSON string value
    void (*number)(void* user_data, const char* str, size_t len); // Called for a JSON number (as string)
    void (*boolean)(void* user_data, bool value);           // Called for true/false
    void (*null)(void* user_data);                          // Called for null
    void (*parse_state_changed)(void* user_data, JSONParserState state); // Called when parser state changes
};
```

- **Notes**:
  - Strings and numbers are provided as pointers to the input buffer with length, not null-terminated, to avoid copying.
  - The `parse_state_changed` handler is optional but useful for monitoring progress.

### JSONParseContext

Holds the persistent state of the parser across resumptions. This is minimal to support resumability.

```C++
struct JSONParseContext {
    // Internal state machine indicator (e.g., current parsing mode: value, object, array, etc.)
    int current_mode;  // 0: root, 1: object, 2: array, etc.
    // Stack for nested structures (dynamically allocated if needed)
    int* stack;
    size_t stack_depth;
    size_t stack_capacity;
    // Error information
    const char* error_msg;
    // User-provided handlers and data
    ParseHandlers handlers;
    void* user_data;
};
```

### JSONParseSession

Represents a single parsing session, including the input buffer and current position.

```C++
struct JSONParseSession {
    JSONParseContext ctx;
    JSONParseHanders handlers;
    const char*      buffer;    // Current input buffer
    size_t           buffer_len;     // Length of the buffer
    size_t           pos;            // Current position in the buffer
    JSONParserState  state; // Current parser state
};
```

## Public API Functions

### init_json_parse_session

Initializes the parsing session.

```C++
struct JSONParseSessionConfig {
    AkU32 max_total_string_size;
    AkU32 max_string_size;
    AkU32 max_nesting;
    AkU32 max_json_size;
    AkU32 max_identifier_size;
};
AkU32               get_required_parse_session_buffer_size(JSONParseSessionConfig* cfg);
JSONParseSession* init_json_parse_session(void* buffer, AkU64 buffer_size, ParseHandlers* handlers, void* user_data);

```

- Sets up the context with handlers and user data.
- Initializes state to `INITIALIZED`.
- Allocates initial stack if needed.

### parse_json

The main entry point for parsing. It processes the provided buffer and can be called multiple times for resumable parsing.

```C++
JSONParserState parse_json(JSONParseSession* session, const char* buffer, size_t len);
```

- Updates the session's buffer and length.
- Starts or resumes parsing from the current position.
- Calls tail-recursive internal functions to process the input.
- Returns the new state: `CONTINUE` if more data is needed, `DONE` if complete, `ERROR` on failure.
- Invokes `parse_state_changed` handler on state changes.

### fini_json_parse_session

Cleans up the session.

```C++
void fini_json_parse_session(JSONParseSession* session);
```

- Frees any allocated resources (e.g., stack).

## Internal Implementation Details

The parser is implemented as a set of tail-recursive functions, each representing a parsing state. These functions take all necessary state as arguments and return by calling the next state function (tail call).

### Design Principles

- **No Local Variables**: All data is passed via arguments.
- **Tail Calls**: Each function ends with a call to another state function, allowing Clang to optimize away the stack frame.
- **State Arguments**: Common arguments include:
  - `JSONParseSession* session`: The session object.
  - `size_t pos`: Current position in the buffer.
  - `int mode`: Current parsing mode (e.g., expecting value, key, etc.).
- **Error Handling**: On error, set state to `ERROR` and return.

### Core Tail-Recursive Functions

To enable tail call optimization, all internal parsing functions share the same signature:

```C++
JSONParserState parse_step(const char* buffer, size_t remaining, size_t pos, int mode, JSONParseContext* ctx);
```

- **buffer**: Pointer to the current input buffer.
- **remaining**: Number of bytes left in the buffer from pos.
- **pos**: Current position in the buffer (absolute index).
- **mode**: Current parsing mode (e.g., 0: root, 1: expecting key in object, 2: expecting value in array, etc.). Define specific constants for modes.
- **ctx**: Pointer to the JSONParseContext for persistent state, stack, handlers, etc.

Each function processes input, updates parameters as needed, and tail-calls another parse_step with updated arguments. If parsing cannot proceed (end of buffer, error, or done), it returns the appropriate JSONParserState.

Instead of separate named functions like parse_root, parse_value, etc., implement them as a single parse_step function with a switch on mode for different behaviors. This ensures uniform tail calls within the same function (self-tail-recursion), which Clang can optimize effectively.

However, for clarity, you can implement them as separate functions with the same signature, each ending with return parse_next(buffer, remaining - consumed, pos + consumed, new_mode, ctx);

Here are the conceptual functions (implement as separate or unified):

1. **parse_root(const char* buffer, size_t remaining, size_t pos, int mode, JSONParseContext* ctx)**: Entry for top-level value.
   - Skip whitespace by tail-calling skip_whitespace with updated pos.
   - Then dispatch to parse_value.

2. **parse_value(const char* buffer, size_t remaining, size_t pos, int mode, JSONParseContext* ctx)**: Parse any value.
   - If remaining == 0, return CONTINUE.
   - Check buffer[pos]:
     - '{' : ctx->handlers.start_object(ctx->user_data); push mode to ctx->stack; tail-call parse_object with mode = MODE_OBJECT_START.
     - Similar for '[', strings, numbers, etc.
   - On error: set ctx->error_msg, return ERROR.

3. **parse_object(const char* buffer, size_t remaining, size_t pos, int mode, JSONParseContext* ctx)**:
   - Depending on sub-mode (e.g., expecting key, colon, value, comma).
   - Advance pos, decrease remaining accordingly.
   - Tail-call appropriate next function.

4. **parse_array(...)**: Similar handling for arrays.

5. **parse_string(const char* buffer, size_t remaining, size_t pos, int start_pos, int mode, JSONParseContext* ctx)**: Note: Added start_pos for collecting string start.
   - Scan until '"', handling escapes.
   - If incomplete (remaining == 0 before closing), return CONTINUE.
   - On complete: call handler with &buffer[start_pos], length = pos - start_pos; tail-call next based on mode.

6. **parse_number(...)**: Similar to parse_string.

7. **skip_whitespace(const char* buffer, size_t remaining, size_t pos, int mode, JSONParseContext* ctx)**:
   - While remaining > 0 and isspace(buffer[pos]), pos++, remaining--.
   - Tail-call the next parse function with updated pos and remaining.

### Additional Notes on Implementation

- **Uniform Signature**: The shared signature allows direct tail calls without stack growth.
- **No Local Variables**: Perform all operations in expressions within the tail call, e.g., return parse_step(buffer, remaining - 1, pos + 1, mode, ctx).
- **State Updates**: Modify ctx for stack pushes/pops. Update session->pos = pos on return from parse_json.
- **Resumption**: When returning CONTINUE, the next parse_json call will start with the new buffer, but you may need to carry over partial token state in ctx (e.g., for mid-string parsing, store start_pos in ctx).

This setup ensures all state is passed via arguments, promoting register usage and tail call optimization.

### Handling Resumability

- When the buffer ends mid-token (e.g., incomplete string), the function returns `CONTINUE`.
- The current `pos` is saved in the session.
- On next call with new buffer, resume from saved `pos` (assuming buffer is appended or continued).

### Nesting with Stack

- Use the context's stack to track nesting levels and modes (object vs array).
- Push mode on start_object/start_array.
- Pop on end_object/end_array.
- Check for proper nesting.

### Whitespace Skipping

- A helper tail-recursive function `skip_whitespace(size_t pos)` that advances pos past spaces, tabs, newlines, etc., then tail-calls the next state.

### Error Conditions

- Unexpected characters.
- Unclosed objects/arrays/strings.
- Invalid numbers.
- Set `session->state = ERROR` and store error message in context.

## Usage Example

```C++
// Define handlers
void my_start_object(void* user_data) { /* ... */ }
// ... other handlers

int main() {
    JSONParseSession session;
    ParseHandlers handlers = { my_start_object, /* ... */ };
    init_json_parse_session(&session, handlers, nullptr);

    const char* json_part1 = "{\"key\": \"val";
    JSONParserState state = parse_json(&session, json_part1, strlen(json_part1));
    // state == CONTINUE

    const char* json_part2 = "ue\"}";
    state = parse_json(&session, json_part2, strlen(json_part2));
    // state == DONE, handlers called accordingly

    fini_json_parse_session(&session);
    return 0;
}
```

### Internal tail recursive routines

```C++
void do_json_dispatch(JSonParseCcontext* )
void do_json_open_object_token()
```

## Implementation

1. **Start with Structures**: Implement the enums and structs exactly as specified.
2. **Init/Fini**: Handle memory for stack in init (e.g., allocate with capacity 16), free in fini.
3. **Tail Recursion**: Ensure every path ends with a return or a tail call to another parse function. Use Clang with -O2 for optimization.
4. **Argument Passing**: Pass `session, pos, mode` to every function. Update `session->pos` only when consuming input.
5. **Handlers**: Call handlers at the right points, passing user_data.
6. **Buffer Management**: Assume buffer is valid until len. For resumption, user must provide continued buffer.
7. **Testing**: Test with valid JSON, invalid, partial inputs, nested structures.
8. **Edge Cases**: Empty objects/arrays, escaped strings, large numbers, unicode.
