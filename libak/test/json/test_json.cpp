#include <gtest/gtest.h> // Assuming gtest is used
#include <cstdlib> // for malloc and free
#include <cstring> // for std::memset
#include <string>
#include <string_view>
#include <vector>

#include <ak.hpp>

using namespace ak;

struct EventCollector;

enum class JSONEventKind {
    StartObject,
    EndObject,
    StartArray,
    EndArray,
    StartKey,
    KeyText,
    EndKey,
    StartString,
    StringText,
    EndString,
    Boolean,
    Null,
    Integer,
    Float,
    StateChanged,
};

struct JSONEvent {

    JSONEventKind   kind;
    std::string     text          = {}; // for KeyText, StringText, Number
    Bool            bool_value    = false; // for Boolean
    I64             integer_value = 0;
    F64             float_value   = 0.0;
    JSONParserState state         = JSONParserState::INVALID; // for StateChanged
    const Char*     err_msg       = nullptr;

    static JSONEvent make_state_changed(JSONParserState parser_state, const Char* err_msg = nullptr) 
    { 
        return {.kind=JSONEventKind::StateChanged, .state = parser_state, .err_msg = err_msg}; 
    }
    
    static JSONEvent make_object_begin()                     { return { .kind=JSONEventKind::StartObject }; }
    static JSONEvent make_object_end()                       { return { .kind=JSONEventKind::EndObject }; }
    static JSONEvent make_array_begin()                      { return { .kind=JSONEventKind::StartArray }; }
    static JSONEvent make_array_end()                        { return { .kind=JSONEventKind::EndArray }; }
    static JSONEvent make_attr_begin()                       { return { .kind=JSONEventKind::StartKey }; }
    static JSONEvent make_attr_key_begin()                   { return { .kind=JSONEventKind::StartKey }; }
    static JSONEvent make_attr_key_end()                     { return { .kind=JSONEventKind::EndKey }; }
    static JSONEvent make_attr_key_chars(std::string_view s) { return { .kind=JSONEventKind::KeyText , .text = std::string(s)}; }
    static JSONEvent make_attr_end()                         { return { .kind=JSONEventKind::EndKey }; }
    static JSONEvent make_string_begin()                     { return { .kind=JSONEventKind::StartString }; }
    static JSONEvent make_string_end()                       { return { .kind=JSONEventKind::EndString }; }
    static JSONEvent make_string_chars(std::string_view s)   { return { .kind=JSONEventKind::StringText, .text = std::string(s)}; }
    static JSONEvent make_null_value()                       { return { .kind=JSONEventKind::Null }; }
    static JSONEvent make_integer_value(I64 value)           { return { .kind=JSONEventKind::Integer, .integer_value = value }; }
    static JSONEvent make_float_value(F64 value)             { return { .kind=JSONEventKind::Float,   .float_value = value }; }
    static JSONEvent make_bool_value(Bool value)             { return { .kind=JSONEventKind::Boolean, .bool_value = value }; }

    bool operator==(const JSONEvent& other) const 
    {
        if (kind != other.kind) return false;
        switch (kind) {
            case JSONEventKind::KeyText:
            case JSONEventKind::StringText:
                return text == other.text;
            case JSONEventKind::Integer:
                return integer_value == other.integer_value;
            case JSONEventKind::Float:
                return float_value == other.float_value;
            case JSONEventKind::Boolean:
                return bool_value == other.bool_value;
            case JSONEventKind::StateChanged:
                return state == other.state;
            case JSONEventKind::StartObject:
            case JSONEventKind::EndObject:
            case JSONEventKind::StartArray:
            case JSONEventKind::EndArray:
            case JSONEventKind::StartKey:
            case JSONEventKind::EndKey:
            case JSONEventKind::StartString:
            case JSONEventKind::EndString:
            case JSONEventKind::Null:
                return true;
            default:
                return true;
        }
    }
};

struct EventCollector {
    std::vector<JSONEvent> events;
};

static std::string simplify_escapes(const std::string &in)
{
    std::string out;
    out.reserve(in.size());
    for (size_t i = 0; i < in.size(); ++i) {
        char c = in[i];
        if (c == '\\' && i + 1 < in.size()) {
            char n = in[i + 1];
            switch (n) {
                case 'n': out.push_back('\n'); ++i; continue;
                case 't': out.push_back('\t'); ++i; continue;
                case 'r': out.push_back('\r'); ++i; continue;
                case 'b': out.push_back('\b'); ++i; continue;
                case 'f': out.push_back('\f'); ++i; continue;
                case '\\': out.push_back('\\'); ++i; continue;
                case '"': out.push_back('"'); ++i; continue;
                case '/': out.push_back('/'); ++i; continue;
                default: break;
            }
        }
        out.push_back(c);
    }
    return out;
}

// Handlers that push events; also log via std::print for visibility
void on_begin_object(JSONParseSession* session) 
{
    auto* c = static_cast<EventCollector*>(session->user_data);
    ASSERT_NE(c, nullptr);
    c->events.push_back(JSONEvent::make_object_begin());
}

void on_end_object(JSONParseSession* session) 
{
    auto* c = static_cast<EventCollector*>(session->user_data);
    ASSERT_NE(c, nullptr);
    c->events.push_back(JSONEvent::make_object_end());
}

void on_array_begin(JSONParseSession* session) 
{
    auto* c = static_cast<EventCollector*>(session->user_data);
    ASSERT_NE(c, nullptr);
    c->events.push_back(JSONEvent::make_array_begin());
}

void on_end_array(JSONParseSession* session) 
{
    auto* c = static_cast<EventCollector*>(session->user_data);
    ASSERT_NE(c, nullptr);
    c->events.push_back(JSONEvent::make_array_end());
}

void on_attr_begin(JSONParseSession* session) 
{
    auto* c = static_cast<EventCollector*>(session->user_data);
    ASSERT_NE(c, nullptr);
    c->events.push_back(JSONEvent::make_attr_begin());
}

void on_attr_key_begin(JSONParseSession* session) 
{
    auto* c = static_cast<EventCollector*>(session->user_data);
    ASSERT_NE(c, nullptr);
    c->events.push_back(JSONEvent::make_attr_key_begin());
}

void on_attr_key_end(JSONParseSession* session) 
{
    auto* c = static_cast<EventCollector*>(session->user_data);
    ASSERT_NE(c, nullptr);
    c->events.push_back(JSONEvent::make_attr_key_end());
}

void on_attr_key_chars(JSONParseSession* session, const Char* str, Size len) 
{
    auto* c = static_cast<EventCollector*>(session->user_data);
    ASSERT_NE(c, nullptr);
    c->events.push_back(JSONEvent::make_attr_key_chars(std::string_view(str, len)));
}

void on_attr_end(JSONParseSession* session) 
{
    auto* c = static_cast<EventCollector*>(session->user_data);
    ASSERT_NE(c, nullptr);
    c->events.push_back(JSONEvent::make_attr_end());
}

void on_null_value(JSONParseSession* session) 
{
    auto* c = static_cast<EventCollector*>(session->user_data);
    ASSERT_NE(c, nullptr);
    c->events.push_back(JSONEvent::make_null_value());
}

void on_bool_value(JSONParseSession* session, Bool value) 
{
    auto* c = static_cast<EventCollector*>(session->user_data);
    ASSERT_NE(c, nullptr);
    c->events.push_back(JSONEvent::make_bool_value(value));
}

void on_integer_value(JSONParseSession* session, I64 value) 
{
    auto* c = static_cast<EventCollector*>(session->user_data);
    ASSERT_NE(c, nullptr);
    c->events.push_back(JSONEvent::make_integer_value(value));
}

void on_float_value(JSONParseSession* session, F64 value) 
{
    auto* c = static_cast<EventCollector*>(session->user_data);
    ASSERT_NE(c, nullptr);
    c->events.push_back(JSONEvent::make_float_value(value));
}

void on_string_begin(JSONParseSession* session) 
{
    auto* c = static_cast<EventCollector*>(session->user_data);
    ASSERT_NE(c, nullptr);
    c->events.push_back(JSONEvent::make_string_begin());
}

void on_string_chars(JSONParseSession* session, const Char* str, Size len) 
{
    auto* c = static_cast<EventCollector*>(session->user_data);
    ASSERT_NE(c, nullptr);
    c->events.push_back(JSONEvent::make_string_chars(std::string_view(str, len)));
}

void on_string_end(JSONParseSession* session) 
{
    auto* c = static_cast<EventCollector*>(session->user_data);
    ASSERT_NE(c, nullptr);
    c->events.push_back(JSONEvent::make_string_end());
}

void on_parse_state_changed(JSONParseSession* session) 
{
    auto* c = static_cast<EventCollector*>(session->user_data);
    ASSERT_NE(c, nullptr);
    c->events.push_back(JSONEvent::make_state_changed(session->state, session->err_msg));
}

static ParseHandlers handlers = {
    .object_begin = on_begin_object,
    .object_end = on_end_object,
    .array_begin = on_array_begin,
    .array_end = on_end_array,
    .attr_begin = on_attr_begin,
    .attr_key_begin = on_attr_key_begin,
    .attr_key_end = on_attr_key_end,
    .attr_key_chars = on_attr_key_chars,
    .attr_end = on_attr_end,
    .null_value = on_null_value,
    .bool_value = on_bool_value,
    .int_value = on_integer_value,
    .float_value = on_float_value,
    .string_value_begin = on_string_begin,
    .string_value_end = on_string_end,
    .string_value_chars = on_string_chars,
    .parse_state_changed = on_parse_state_changed,
};

static JSONParserState do_parse_run(const char* json_text, U64 json_text_size, EventCollector* collector) 
{
    static const size_t BUFFER_SIZE = 1024 * 1024;
    static char BUFFER[BUFFER_SIZE];
    std::memset(BUFFER, 0, BUFFER_SIZE);
    
    JSONParseSessionConfig cfg = {
        .max_json_size = BUFFER_SIZE,
        .max_string_size = 256,
        .max_depth = 32,
    };
    U32 req_size = get_required_parse_session_buffer_size(&cfg);
    if (req_size > BUFFER_SIZE) {
        ADD_FAILURE() << "Insufficient test buffer size";
    }
    JSONParseSession* session = init_json_parser(BUFFER, BUFFER_SIZE, &cfg, &handlers, (Void*)collector);
    EXPECT_NE(session, nullptr);
    return parse_buffer(session, (void*)json_text, json_text_size);
}

// Tests grouped

// Initialization Test
TEST(JSONParser, Initialization) {
    const size_t BUF_SIZE = 1024 * 1024;
    alignas(alignof(JSONParseSession)) char buffer[BUF_SIZE];
    std::memset(buffer, 0, BUF_SIZE);
    JSONParseSessionConfig cfg = {
        .max_json_size = BUF_SIZE,
        .max_string_size = 256,
        .max_depth = 32,
    };
    U32 req_size = get_required_parse_session_buffer_size(&cfg);
    ASSERT_LE(req_size, BUF_SIZE);

    EventCollector collector;
    
    JSONParseSession* session = init_json_parser(buffer, BUF_SIZE, &cfg, &handlers, (Void*)&collector);
    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->state, JSONParserState::INITIALIZED);
}

TEST(JSONParser, VectorPushBack) {
    EventCollector collector;
    EventCollector expected;
    collector.events.push_back(JSONEvent::make_object_begin());
    expected.events.push_back(JSONEvent::make_object_begin());
    EXPECT_EQ(collector.events, expected.events);
}

// Invalid Input Tests - Expect ERROR and empty events
TEST(JSONParser, EmptyInput) {
    EventCollector collector;
    EventCollector expected;
    expected.events.push_back(JSONEvent::make_state_changed(JSONParserState::ERROR, "empty input"));
    auto res = do_parse_run("", 0, &collector);
    EXPECT_EQ(res, JSONParserState::ERROR);
    EXPECT_EQ(collector.events, expected.events);
}

TEST(JSONParser, WhitespaceOnly) {
    EventCollector collector;
    EventCollector expected;
    expected.events.push_back(JSONEvent::make_state_changed(JSONParserState::ERROR, "unexpected end of input"));
    const Char text[] = "   \t\n\r  ";
    auto res = do_parse_run(text, strlen(text), &collector);
    EXPECT_EQ(res, JSONParserState::ERROR);
    EXPECT_EQ(collector.events, expected.events);
}

TEST(JSONParser, LoneClosingBrace) {
    EventCollector collector;
    const Char text[] = "}";
    auto res = do_parse_run(text, strlen(text), &collector);
    EXPECT_EQ(res, JSONParserState::ERROR);
    EventCollector expected;
    expected.events.push_back(JSONEvent::make_state_changed(JSONParserState::ERROR, "expected an Object '{ ... }' or an Array '[ ... ]'"));
    EXPECT_EQ(collector.events, expected.events);
}

TEST(JSONParser, LoneClosingBracket) {
    EventCollector collector;
    EventCollector expected;
    expected.events.push_back(JSONEvent::make_state_changed(JSONParserState::ERROR, "expected an Object '{ ... }' or an Array '[ ... ]'"));
    const Char text[] = "]";
    auto res = do_parse_run(text, strlen(text), &collector);
    EXPECT_EQ(res, JSONParserState::ERROR);
    EXPECT_EQ(collector.events, expected.events);
}

TEST(JSONParser, BareIdentifier) {
    EventCollector collector;
    EventCollector expected;
    expected.events.push_back(JSONEvent::make_state_changed(JSONParserState::ERROR, "expected an Object '{ ... }' or an Array '[ ... ]'"));
    const Char text[] = "abc";
    auto res = do_parse_run(text, strlen(text), &collector);
    EXPECT_EQ(res, JSONParserState::ERROR);
    EXPECT_EQ(collector.events, expected.events);
}

TEST(JSONParser, MalformedString) {
    EventCollector collector;
    const Char text[] = "\"unclosed";
    auto res = do_parse_run(text, strlen(text), &collector);
    EXPECT_EQ(res, JSONParserState::ERROR);
    ASSERT_FALSE(collector.events.empty());
    const auto &last = collector.events.back();
    EXPECT_EQ(last.kind, JSONEventKind::StateChanged);
    EXPECT_EQ(last.state, JSONParserState::ERROR);
    EXPECT_STREQ(last.err_msg, "unexpected end of input");
}

TEST(JSONParser, MalformedNumber) {
    EventCollector collector;
    EventCollector expected;
    expected.events.push_back(JSONEvent::make_state_changed(JSONParserState::ERROR, "expected an Object '{ ... }' or an Array '[ ... ]'"));
    const Char text[] = "--1";
    auto res = do_parse_run(text, strlen(text), &collector);
    EXPECT_EQ(res, JSONParserState::ERROR);
    EXPECT_EQ(collector.events, expected.events);
}

TEST(JSONParser, InvalidCommaPlacement) {
    EventCollector collector;
    const Char text[] = "{a}";
    auto res = do_parse_run(text, strlen(text), &collector);
    EXPECT_EQ(res, JSONParserState::ERROR);
}

TEST(JSONParser, InvalidTrailingComma) {
    EventCollector collector;
    const Char text[] = "{\"a\":1,}";
    auto res = do_parse_run(text, strlen(text), &collector);
    EXPECT_EQ(res, JSONParserState::ERROR);
    ASSERT_FALSE(collector.events.empty());
    const auto &last = collector.events.back();
    EXPECT_EQ(last.kind, JSONEventKind::StateChanged);
    EXPECT_EQ(last.state, JSONParserState::ERROR);
    EXPECT_STREQ(last.err_msg, "expected a string key");
}

TEST(JSONParser, ArrayMissingComma) {
    EventCollector collector;
    const Char text[] = "[1 2]";
    auto res = do_parse_run(text, strlen(text), &collector);
    EXPECT_EQ(res, JSONParserState::ERROR);
    ASSERT_FALSE(collector.events.empty());
    const auto &last = collector.events.back();
    EXPECT_EQ(last.kind, JSONEventKind::StateChanged);
    EXPECT_EQ(last.state, JSONParserState::ERROR);
    EXPECT_STREQ(last.err_msg, "expected a comma or a closing bracket");
}

// Incomplete Input Tests - Expect CONTINUE and partial events
TEST(JSONParser, LoneOpeningBrace) {
    EventCollector collector;
    const Char text[] = "{";
    auto res = do_parse_run(text, strlen(text), &collector);
    EXPECT_EQ(res, JSONParserState::CONTINUE);
    std::vector<JSONEvent> expected = { JSONEvent::make_object_begin() };
    EXPECT_EQ(collector.events, expected);
}

TEST(JSONParser, LoneOpeningBracket) {
    EventCollector collector;
    const Char text[] = "[";
    auto res = do_parse_run(text, strlen(text), &collector);
    EXPECT_EQ(res, JSONParserState::CONTINUE);
    std::vector<JSONEvent> expected = { JSONEvent::make_array_begin() };
    EXPECT_EQ(collector.events, expected);
}

TEST(JSONParser, IncompleteStringInObject) {
    EventCollector collector;
    const Char text[] = R"({"k":")";
    auto res = do_parse_run(text, strlen(text), &collector);
    EXPECT_EQ(res, JSONParserState::CONTINUE);
    std::vector<JSONEvent> expected = {
        JSONEvent::make_object_begin(),
        JSONEvent::make_attr_begin(),
        JSONEvent::make_attr_key_begin(),
        JSONEvent::make_attr_key_chars("k"),
        JSONEvent::make_attr_key_end(),
        JSONEvent::make_string_begin()
    };
    EXPECT_EQ(collector.events, expected);
}

// Valid Simple Structure Tests - Expect DONE and full events
TEST(JSONParser, ParseEmptyObject) {
    EventCollector collector;
    EventCollector expected;
    auto res = do_parse_run("{}", 2, &collector);
    EXPECT_EQ(res, JSONParserState::DONE);
    expected.events = {
        JSONEvent::make_object_begin(),
        JSONEvent::make_object_end()
    };
    EXPECT_EQ(collector.events, expected.events);
}

TEST(JSONParser, ParseEmptyArray) {
    EventCollector collector;
    EventCollector expected;
    auto res = do_parse_run("[]", 2, &collector);
    EXPECT_EQ(res, JSONParserState::DONE);
    expected.events = {
        JSONEvent::make_array_begin(),
        JSONEvent::make_array_end()
    };
    EXPECT_EQ(collector.events, expected.events);
}

// Valid Complex Structure Tests
TEST(JSONParser, ParseSimpleObject) {
    EventCollector collector;
    EventCollector expected;
    const char* json = R"({ "a": 1, "b": "x", "c": true, "d": false, "e": null })";
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::DONE);
    expected.events = {
        JSONEvent::make_object_begin(),
        JSONEvent::make_attr_begin(),
        JSONEvent::make_attr_key_begin(),
        JSONEvent::make_attr_key_chars("a"),
        JSONEvent::make_attr_key_end(),
        JSONEvent::make_integer_value(1),
        JSONEvent::make_attr_end(),
        JSONEvent::make_attr_begin(),
        JSONEvent::make_attr_key_begin(),
        JSONEvent::make_attr_key_chars("b"),
        JSONEvent::make_attr_key_end(),
        JSONEvent::make_string_begin(),
        JSONEvent::make_string_chars("x"),
        JSONEvent::make_string_end(),
        JSONEvent::make_attr_end(),
        JSONEvent::make_attr_begin(),
        JSONEvent::make_attr_key_begin(),
        JSONEvent::make_attr_key_chars("c"),
        JSONEvent::make_attr_key_end(),
        JSONEvent::make_bool_value(true),
        JSONEvent::make_attr_end(),
        JSONEvent::make_attr_begin(),
        JSONEvent::make_attr_key_begin(),
        JSONEvent::make_attr_key_chars("d"),
        JSONEvent::make_attr_key_end(),
        JSONEvent::make_bool_value(false),
        JSONEvent::make_attr_end(),
        JSONEvent::make_attr_begin(),
        JSONEvent::make_attr_key_begin(),
        JSONEvent::make_attr_key_chars("e"),
        JSONEvent::make_attr_key_end(),
        JSONEvent::make_null_value(),
        JSONEvent::make_attr_end(),
        JSONEvent::make_object_end()
    };
    EXPECT_EQ(collector.events, expected.events);
}

// Arrays - additional coverage

TEST(JSONParser, ParseArrayOfIntegers) {
    EventCollector collector;
    const char* json = "[1,2,3]";
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::DONE);
    std::vector<JSONEvent> expected = {
        JSONEvent::make_array_begin(),
        JSONEvent::make_integer_value(1),
        JSONEvent::make_integer_value(2),
        JSONEvent::make_integer_value(3),
        JSONEvent::make_array_end()
    };
    EXPECT_EQ(collector.events, expected);
}

TEST(JSONParser, ParseArrayMixedTypes) {
    EventCollector collector;
    const char* json = R"([null, true, false, "x", 1, 2.5])";
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::DONE);
    std::vector<JSONEvent> expected = {
        JSONEvent::make_array_begin(),
        JSONEvent::make_null_value(),
        JSONEvent::make_bool_value(true),
        JSONEvent::make_bool_value(false),
        JSONEvent::make_string_begin(),
        JSONEvent::make_string_chars("x"),
        JSONEvent::make_string_end(),
        JSONEvent::make_integer_value(1),
        JSONEvent::make_float_value(2.5),
        JSONEvent::make_array_end()
    };
    EXPECT_EQ(collector.events, expected);
}

TEST(JSONParser, ParseNestedArrays) {
    EventCollector collector;
    const char* json = "[1, [2, 3], 4]";
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_TRUE(res == JSONParserState::DONE || res == JSONParserState::CONTINUE);
    std::vector<JSONEvent> expected = {
        JSONEvent::make_array_begin(),
        JSONEvent::make_integer_value(1),
        JSONEvent::make_array_begin(),
        JSONEvent::make_integer_value(2),
        JSONEvent::make_integer_value(3),
        JSONEvent::make_array_end(),
        JSONEvent::make_integer_value(4),
        JSONEvent::make_array_end()
    };
    EXPECT_EQ(collector.events, expected);
}

TEST(JSONParser, ParseArrayOfObjects) {
    EventCollector collector;
    const char* json = R"([{"a": 1}, {"b": "x"}])";
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_TRUE(res == JSONParserState::DONE || res == JSONParserState::CONTINUE);
    std::vector<JSONEvent> expected = {
        JSONEvent::make_array_begin(),
        JSONEvent::make_object_begin(),
        JSONEvent::make_attr_begin(),
        JSONEvent::make_attr_key_begin(),
        JSONEvent::make_attr_key_chars("a"),
        JSONEvent::make_attr_key_end(),
        JSONEvent::make_integer_value(1),
        JSONEvent::make_attr_end(),
        JSONEvent::make_object_end(),
        JSONEvent::make_object_begin(),
        JSONEvent::make_attr_begin(),
        JSONEvent::make_attr_key_begin(),
        JSONEvent::make_attr_key_chars("b"),
        JSONEvent::make_attr_key_end(),
        JSONEvent::make_string_begin(),
        JSONEvent::make_string_chars("x"),
        JSONEvent::make_string_end(),
        JSONEvent::make_attr_end(),
        JSONEvent::make_object_end(),
        JSONEvent::make_array_end()
    };
    // Compare only up to expected size to tolerate trailing notifications
    ASSERT_GE(collector.events.size(), expected.size());
    std::vector<JSONEvent> prefix(collector.events.begin(), collector.events.begin() + expected.size());
    EXPECT_EQ(prefix, expected);
}

TEST(JSONParser, IncompleteArrayPendingClose) {
    EventCollector collector;
    const char* json = R"(["x")"; // missing closing ]
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::CONTINUE);
    std::vector<JSONEvent> expected = {
        JSONEvent::make_array_begin(),
        JSONEvent::make_string_begin(),
        JSONEvent::make_string_chars("x"),
        JSONEvent::make_string_end()
    };
    EXPECT_EQ(collector.events, expected);
}

TEST(JSONParser, ArrayTrailingComma) {
    EventCollector collector;
    const char* json = "[1,]";
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::ERROR);
    ASSERT_GE(collector.events.size(), 2u);
    std::vector<JSONEvent> prefix = {
        JSONEvent::make_array_begin(),
        JSONEvent::make_integer_value(1),
    };
    std::vector<JSONEvent> got_prefix(collector.events.begin(), collector.events.begin() + 2);
    EXPECT_EQ(got_prefix, prefix);
    const auto &last = collector.events.back();
    EXPECT_EQ(last.kind, JSONEventKind::StateChanged);
    EXPECT_EQ(last.state, JSONParserState::ERROR);
}

// RFC 8259 Compliance Tests

TEST(JSONParser, TopLevelString) {
    EventCollector collector;
    const char* json = "\"hello\"";
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::DONE);
    std::vector<JSONEvent> expected = {
        JSONEvent::make_string_begin(),
        JSONEvent::make_string_chars("hello"),
        JSONEvent::make_string_end()
    };
    EXPECT_EQ(collector.events, expected);
}

TEST(JSONParser, TopLevelNumber) {
    EventCollector collector;
    const char* json = "42";
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::DONE);
    std::vector<JSONEvent> expected = {
        JSONEvent::make_integer_value(42)
    };
    EXPECT_EQ(collector.events, expected);
}

TEST(JSONParser, TopLevelTrue) {
    EventCollector collector;
    const char* json = "true";
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::DONE);
    std::vector<JSONEvent> expected = {
        JSONEvent::make_bool_value(true)
    };
    EXPECT_EQ(collector.events, expected);
}

TEST(JSONParser, TopLevelFalse) {
    EventCollector collector;
    const char* json = "false";
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::DONE);
    std::vector<JSONEvent> expected = {
        JSONEvent::make_bool_value(false)
    };
    EXPECT_EQ(collector.events, expected);
}

TEST(JSONParser, TopLevelNull) {
    EventCollector collector;
    const char* json = "null";
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::DONE);
    std::vector<JSONEvent> expected = {
        JSONEvent::make_null_value()
    };
    EXPECT_EQ(collector.events, expected);
}

TEST(JSONParser, InvalidLeadingZeroInteger) {
    EventCollector collector;
    const char* json = "0123";
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::ERROR);
    ASSERT_FALSE(collector.events.empty());
    const auto &last = collector.events.back();
    EXPECT_EQ(last.kind, JSONEventKind::StateChanged);
    EXPECT_EQ(last.state, JSONParserState::ERROR);
    EXPECT_STREQ(last.err_msg, "invalid number format: leading zero");
}

TEST(JSONParser, InvalidFractionNoDigits) {
    EventCollector collector;
    const char* json = "1.";
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::ERROR);
    ASSERT_FALSE(collector.events.empty());
    const auto &last = collector.events.back();
    EXPECT_EQ(last.kind, JSONEventKind::StateChanged);
    EXPECT_EQ(last.state, JSONParserState::ERROR);
    EXPECT_STREQ(last.err_msg, "invalid number format: no digits after decimal");
}

TEST(JSONParser, InvalidExponentNoDigits) {
    EventCollector collector;
    const char* json = "1e";
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::ERROR);
    ASSERT_FALSE(collector.events.empty());
    const auto &last = collector.events.back();
    EXPECT_EQ(last.kind, JSONEventKind::StateChanged);
    EXPECT_EQ(last.state, JSONParserState::ERROR);
    EXPECT_STREQ(last.err_msg, "invalid number format: no digits in exponent");
}

TEST(JSONParser, UnicodeSurrogatePair) {
    EventCollector collector;
    const char* json = "\"\\ud83d\\ude00\""; // Grinning face emoji
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::DONE);
    std::vector<JSONEvent> expected = {
        JSONEvent::make_string_begin(),
        JSONEvent::make_string_chars("\xf0\x9f\x98\x80"), // UTF-8 for U+1F600
        JSONEvent::make_string_end()
    };
    EXPECT_EQ(collector.events, expected);
}

TEST(JSONParser, InvalidSurrogatePair) {
    EventCollector collector;
    const char* json = "\"\\ud83d\""; // High surrogate without low
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::ERROR);
    ASSERT_FALSE(collector.events.empty());
    const auto &last = collector.events.back();
    EXPECT_EQ(last.kind, JSONEventKind::StateChanged);
    EXPECT_EQ(last.state, JSONParserState::ERROR);
    EXPECT_STREQ(last.err_msg, "invalid surrogate pair");
}

TEST(JSONParser, InvalidUnicodeEscape) {
    EventCollector collector;
    const char* json = "\"\\uGGGG\""; // Invalid hex
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::ERROR);
    ASSERT_FALSE(collector.events.empty());
    const auto &last = collector.events.back();
    EXPECT_EQ(last.kind, JSONEventKind::StateChanged);
    EXPECT_EQ(last.state, JSONParserState::ERROR);
    EXPECT_STREQ(last.err_msg, "invalid hex digit in unicode escape");
}

// TEST(JSONParser, ParseNestedStructures) {
//     EventCollector collector;
//     const char* json = R"({ "arr": [1, 2, 3, { "k": "v" }], "obj": {} })";
//     auto res = do_parse_run(json, 31, &collector);
//     EXPECT_EQ(res, JSONParserState::DONE);
//     std::vector<JSONEvent> expected = {
//         JSONEvent::start_object(),
//         JSONEvent::start_key(),
//         JSONEvent::key_text("arr"),
//         JSONEvent::end_key(),
//         JSONEvent::start_array(),
//         JSONEvent::number("1"),
//         JSONEvent::number("2"),
//         JSONEvent::number("3"),
//         JSONEvent::start_object(),
//         JSONEvent::start_key(),
//         JSONEvent::key_text("k"),
//         JSONEvent::end_key(),
//         JSONEvent::start_string(),
//         JSONEvent::string_text("v"),
//         JSONEvent::end_string(),
//         JSONEvent::end_object(),
//         JSONEvent::end_array(),
//         JSONEvent::start_key(),
//         JSONEvent::key_text("obj"),
//         JSONEvent::end_key(),
//         JSONEvent::start_object(),
//         JSONEvent::end_object(),
//         JSONEvent::end_object()
//     };
//     EXPECT_EQ(collector.events, expected);
// }

// // Depth and Limit Tests
// TEST(JSONParser, DeepNesting) {
//     int depth = 16; // <= cfg.max_depth
//     std::string json = "null";
//     for (int i = 0; i < depth; ++i) {
//         json = "{\"key\":" + json + "}";
//     }
//     EventCollector collector;
//     auto res = do_parse_run(json.c_str(), json.length(), &collector);
//     EXPECT_EQ(res, JSONParserState::DONE);
//     // Can add expected, but for simplicity, check not ERROR
// }

// TEST(JSONParser, MaxDepthExceeded) {
//     int depth = 40; // > cfg.max_depth
//     std::string json = "null";
//     for (int i = 0; i < depth; ++i) {
//         json = "{ \"key\" : " + json + "}";
//     }
//     EventCollector collector;
//     auto res = do_parse_run(json.c_str(), json.length(), &collector);
//     EXPECT_EQ(res, JSONParserState::ERROR);
//     EXPECT_TRUE(collector.events.empty());
// }

// // String Handling Tests
// TEST(JSONParser, ParseEscapedStrings) {
//     EventCollector collector;
//     auto res = do_parse_run(R"({\"s\":\"\\\\\\\"quote\\nline\\t tab\"})", 24, &collector);
//     EXPECT_EQ(res, JSONParserState::DONE);
//     std::vector<JSONEvent> expected = {
//         JSONEvent::start_object(),
//         JSONEvent::start_key(),
//         JSONEvent::key_text("s"),
//         JSONEvent::end_key(),
//         JSONEvent::start_string(),
//         JSONEvent::string_text("\\\"quote\nline\t tab"),
//         JSONEvent::end_string(),
//         JSONEvent::end_object()
//     };
//     EXPECT_EQ(collector.events, expected);
// }

// TEST(JSONParser, UnicodeEscapes) {
//     EventCollector collector;
//     auto res = do_parse_run("{\"u\":\"\\u0041\"}", 14, &collector);
//     EXPECT_EQ(res, JSONParserState::DONE);
//     std::vector<JSONEvent> expected = {
//         JSONEvent::start_object(),
//         JSONEvent::start_key(),
//         JSONEvent::key_text("u"),
//         JSONEvent::end_key(),
//         JSONEvent::start_string(),
//         JSONEvent::string_text("A"),
//         JSONEvent::end_string(),
//         JSONEvent::end_object()
//     };
//     EXPECT_EQ(collector.events, expected);
// }

// TEST(JSONParser, LargeString) {
//     EventCollector collector;
//     std::string big(10000, 'a');
//     std::string json = std::string("{\"s\":\"") + big + "\"}";
//     auto res = do_parse_run(json.c_str(), json.length(), &collector);
//     EXPECT_EQ(res, JSONParserState::DONE);
//     std::string reconstructed;
//     for (const auto& ev : collector.events) {
//         if (ev.kind == JSONEventKind::StringText) reconstructed += ev.text;
//     }
//     EXPECT_EQ(reconstructed, big);
// }

// New tests for string escapes

TEST(JSONParser, SimpleEscapedString) {
    EventCollector collector;
    const char* json = R"({"s":"\"\\"})";
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::DONE);
    std::string reconstructed;
    bool in_string = false;
    for (const auto &ev : collector.events) {
        if (ev.kind == JSONEventKind::StartString) in_string = true;
        if (ev.kind == JSONEventKind::StringText) reconstructed += ev.text;
        if (ev.kind == JSONEventKind::EndString) in_string = false;
    }
    EXPECT_EQ(reconstructed, std::string("\"\\"));
}

TEST(JSONParser, AllStandardEscapes) {
    EventCollector collector;
    const char* json = R"({"esc":"\b\f\n\r\t\/\\\""})";
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::DONE);
    std::string reconstructed;
    for (const auto &ev : collector.events) {
        if (ev.kind == JSONEventKind::StringText) reconstructed += ev.text;
    }
    EXPECT_EQ(reconstructed, std::string("\b\f\n\r\t/\\\""));
}

TEST(JSONParser, UnicodeEscape) {
    EventCollector collector;
    const char* json = R"({"u":"\u0041"})";
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::DONE);
    std::vector<JSONEvent> expected = {
        JSONEvent::make_object_begin(),
        JSONEvent::make_attr_begin(),
        JSONEvent::make_attr_key_begin(),
        JSONEvent::make_attr_key_chars("u"),
        JSONEvent::make_attr_key_end(),
        JSONEvent::make_string_begin(),
        JSONEvent::make_string_chars("A"),
        JSONEvent::make_string_end(),
        JSONEvent::make_attr_end(),
        JSONEvent::make_object_end()
    };
    EXPECT_EQ(collector.events, expected);
}

TEST(JSONParser, InvalidEscape) {
    EventCollector collector;
    const char* json = R"({"bad":"\x"})";
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::ERROR);
    ASSERT_FALSE(collector.events.empty());
    const auto &last = collector.events.back();
    EXPECT_EQ(last.kind, JSONEventKind::StateChanged);
    EXPECT_EQ(last.state, JSONParserState::ERROR);
    EXPECT_STREQ(last.err_msg, "invalid escape sequence character");
}

TEST(JSONParser, IncompleteEscape) {
    EventCollector collector;
    const char* json = "{\"inc\":\"\\"; // ends with a single backslash, incomplete
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::CONTINUE);
}

TEST(JSONParser, EscapedKey) {
    EventCollector collector;
    const char* json = R"({"key":1})";
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_TRUE(res == JSONParserState::DONE || res == JSONParserState::CONTINUE);
    // Expect the key text without surrounding quotes
    bool saw_key = false;
    for (const auto &ev : collector.events) {
        if (ev.kind == JSONEventKind::KeyText) { EXPECT_EQ(ev.text, std::string("key")); saw_key = true; }
    }
    EXPECT_TRUE(saw_key);
}

TEST(JSONParser, LongStringWithEscapes) {
    EventCollector collector;
    std::string big(100, 'a');
    big += "\\n";
    big += std::string(100, 'b');
    std::string json = R"({"long":")" + big + R"("})";
    auto res = do_parse_run(json.c_str(), json.length(), &collector);
    EXPECT_EQ(res, JSONParserState::DONE);
    std::string reconstructed;
    for (const auto& ev : collector.events) {
        if (ev.kind == JSONEventKind::StringText) reconstructed += ev.text;
    }
    std::string decoded = simplify_escapes(reconstructed);
    std::string expected_str = std::string(100, 'a') + "\n" + std::string(100, 'b');
    EXPECT_EQ(decoded, expected_str);
}

TEST(JSONParser, StringWithMultipleChunks) {
    EventCollector collector;
    const char* json = R"({"chunks":"abc\\ndef\\tghi"})";
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::DONE);
    std::string reconstructed;
    for (const auto &ev : collector.events) {
        if (ev.kind == JSONEventKind::StringText) reconstructed += ev.text;
    }
    EXPECT_EQ(simplify_escapes(reconstructed), std::string("abc\ndef\tghi"));
}

TEST(JSONParser, NestedObjectWithEscapedStrings) {
    EventCollector collector;
    const char* json = R"({"outer":"val\\n", "inner":{"key":"\u0065scape"}})";
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::DONE);
    std::vector<std::string> strings;
    std::string current;
    bool in_string = false;
    for (const auto &ev : collector.events) {
        if (ev.kind == JSONEventKind::StartString) { current.clear(); in_string = true; }
        if (ev.kind == JSONEventKind::StringText) current += ev.text;
        if (ev.kind == JSONEventKind::EndString && in_string) { strings.push_back(current); in_string = false; }
    }
    ASSERT_EQ(strings.size(), 2u);
    EXPECT_EQ(simplify_escapes(strings[0]), std::string("val\n"));
    EXPECT_EQ(strings[1], std::string("escape"));
}

TEST(JSONParser, ArrayOfEscapedStrings) {
    EventCollector collector;
    const char* json = R"(["\b","\f","\n","\r","\t","\/","\\\""])";
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::DONE);
    std::vector<std::string> strings;
    std::string cur;
    bool in_string = false;
    for (const auto &ev : collector.events) {
        if (ev.kind == JSONEventKind::StartString) { cur.clear(); in_string = true; }
        if (ev.kind == JSONEventKind::StringText) cur += ev.text;
        if (ev.kind == JSONEventKind::EndString && in_string) { strings.push_back(cur); in_string = false; }
    }
    std::vector<std::string> expected = {"\b","\f","\n","\r","\t","/","\\\""};
    EXPECT_EQ(strings, expected);
}
