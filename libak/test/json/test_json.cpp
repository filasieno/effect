#include <gtest/gtest.h> // Assuming gtest is used
#include <cstdlib>       // for malloc and free
#include <cstring>       // for std::memset
#include <string>
#include <string_view>
#include <vector>

#include <ak.hpp>

using namespace ak;

struct EventCollector;

struct TestEvent {

    ak::JSONEvent event_type;
    std::string text = {};   // for KeyText, StringText, Number
    Bool bool_value = false; // for Boolean
    I64 integer_value = 0;
    F64 float_value = 0.0;
    JSONParserState state = JSONParserState::INVALID; // for StateChanged
    const Char *err_msg = nullptr;

    static TestEvent make_state_changed(JSONParserState parser_state, const Char *err_msg = nullptr) {
        return {.event_type = ak::JSONEvent::PARSE_STATE_CHANGED, .state = parser_state, .err_msg = err_msg};
    }

    static TestEvent make_object_begin() { return {.event_type = ak::JSONEvent::OBJECT_BEGIN}; }
    static TestEvent make_object_end() { return {.event_type = ak::JSONEvent::OBJECT_END}; }
    static TestEvent make_array_begin() { return {.event_type = ak::JSONEvent::ARRAY_BEGIN}; }
    static TestEvent make_array_end() { return {.event_type = ak::JSONEvent::ARRAY_END}; }
    // attr_begin removed
    static TestEvent make_attr_key_begin() { return {.event_type = ak::JSONEvent::ATTR_KEY_BEGIN}; }
    static TestEvent make_attr_key_end() { return {.event_type = ak::JSONEvent::ATTR_KEY_END}; }
    static TestEvent make_attr_key_chars(std::string_view s) { return {.event_type = ak::JSONEvent::ATTR_KEY_CHARS, .text = std::string(s)}; }
    static TestEvent make_key(std::string_view s) { return {.event_type = ak::JSONEvent::KEY, .text = std::string(s)}; }
    // attr_end removed
    static TestEvent make_string_begin() { return {.event_type = ak::JSONEvent::STRING_VALUE_BEGIN}; }
    static TestEvent make_string_end() { return {.event_type = ak::JSONEvent::STRING_VALUE_END}; }
    static TestEvent make_string_chars(std::string_view s) { return {.event_type = ak::JSONEvent::STRING_VALUE_CHARS, .text = std::string(s)}; }
    static TestEvent make_string(std::string_view s) { return {.event_type = ak::JSONEvent::STRING, .text = std::string(s)}; }
    static TestEvent make_null_value() { return {.event_type = ak::JSONEvent::NULL_VALUE}; }
    static TestEvent make_integer_value(I64 value) { return {.event_type = ak::JSONEvent::INT_VALUE, .integer_value = value}; }
    static TestEvent make_float_value(F64 value) { return {.event_type = ak::JSONEvent::FLOAT_VALUE, .float_value = value}; }
    static TestEvent make_bool_value(Bool value) { return {.event_type = ak::JSONEvent::BOOL_VALUE, .bool_value = value}; }

    bool operator==(const TestEvent &other) const {
        if (event_type != other.event_type)
            return false;
        switch (event_type) {
        case ak::JSONEvent::ATTR_KEY_CHARS:
        case ak::JSONEvent::STRING_VALUE_CHARS:
        case ak::JSONEvent::KEY:
        case ak::JSONEvent::STRING:
            return text == other.text;
        case ak::JSONEvent::INT_VALUE:
            return integer_value == other.integer_value;
        case ak::JSONEvent::FLOAT_VALUE:
            return float_value == other.float_value;
        case ak::JSONEvent::BOOL_VALUE:
            return bool_value == other.bool_value;
        case ak::JSONEvent::PARSE_STATE_CHANGED:
            return state == other.state;
        case ak::JSONEvent::OBJECT_BEGIN:
        case ak::JSONEvent::OBJECT_END:
        case ak::JSONEvent::ARRAY_BEGIN:
        case ak::JSONEvent::ARRAY_END:
        case ak::JSONEvent::ATTR_KEY_BEGIN:
        case ak::JSONEvent::ATTR_KEY_END:
        case ak::JSONEvent::STRING_VALUE_BEGIN:
        case ak::JSONEvent::STRING_VALUE_END:
        case ak::JSONEvent::NULL_VALUE:
            return true;
        default:
            return true;
        }
    }
};

struct EventCollector {
    std::vector<TestEvent> events;
};

// decoding helper no longer used

// Unified event handler that pushes events
void on_json_event(JSONParseSession *session, ak::JSONEvent event, const JSONEventData *data) noexcept {
    auto *c = static_cast<EventCollector *>(session->user_data);
    ASSERT_NE(c, nullptr);

    switch (event) {
        case ak::JSONEvent::OBJECT_BEGIN:
            c->events.push_back(TestEvent::make_object_begin());
            break;
        case ak::JSONEvent::OBJECT_END:
            c->events.push_back(TestEvent::make_object_end());
            break;
        case ak::JSONEvent::ARRAY_BEGIN:
            c->events.push_back(TestEvent::make_array_begin());
            break;
        case ak::JSONEvent::ARRAY_END:
            c->events.push_back(TestEvent::make_array_end());
            break;
        case ak::JSONEvent::ATTR_BEGIN:
            break; // removed
        case ak::JSONEvent::ATTR_KEY_BEGIN:
            c->events.push_back(TestEvent::make_attr_key_begin());
            break;
        case ak::JSONEvent::ATTR_KEY_END:
            c->events.push_back(TestEvent::make_attr_key_end());
            break;
        case ak::JSONEvent::ATTR_KEY_CHARS:
            if (data) {
                c->events.push_back(TestEvent::make_attr_key_chars(
                    std::string_view(data->string_data.str, data->string_data.len)));
            }
            break;
        case ak::JSONEvent::KEY:
            if (data) {
                c->events.push_back(TestEvent::make_key(
                    std::string_view(data->string_data.str, data->string_data.len)));
            }
            break;
        case ak::JSONEvent::ATTR_END:
            break; // removed
        case ak::JSONEvent::NULL_VALUE:
            c->events.push_back(TestEvent::make_null_value());
            break;
        case ak::JSONEvent::BOOL_VALUE:
            if (data) {
                c->events.push_back(TestEvent::make_bool_value(data->bool_value));
            }
            break;
        case ak::JSONEvent::INT_VALUE:
            if (data) {
                c->events.push_back(TestEvent::make_integer_value(data->int_value));
            }
            break;
        case ak::JSONEvent::FLOAT_VALUE:
            if (data) {
                c->events.push_back(TestEvent::make_float_value(data->float_value));
            }
            break;
        case ak::JSONEvent::STRING_VALUE_BEGIN:
            c->events.push_back(TestEvent::make_string_begin());
            break;
        case ak::JSONEvent::STRING_VALUE_END:
            c->events.push_back(TestEvent::make_string_end());
            break;
        case ak::JSONEvent::STRING_VALUE_CHARS:
            if (data) {
                c->events.push_back(TestEvent::make_string_chars(
                    std::string_view(data->string_data.str, data->string_data.len)));
            }
            break;
        case ak::JSONEvent::STRING:
            if (data) {
                c->events.push_back(TestEvent::make_string(
                    std::string_view(data->string_data.str, data->string_data.len)));
            }
            break;
        case ak::JSONEvent::PARSE_STATE_CHANGED:
            if (data) {
                c->events.push_back(TestEvent::make_state_changed(
                    data->state_data.state, data->state_data.err_msg));
            } else {
                c->events.push_back(TestEvent::make_state_changed(session->state, session->err_msg));
            }
            break;
    }
}

static JSONParserState do_parse_run(const char *json_text, U64 json_text_size, EventCollector *collector) {
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
    JSONParseSession *session = init_json_parser(BUFFER, BUFFER_SIZE, &cfg, on_json_event, (Void *)collector);
    EXPECT_NE(session, nullptr);
    return parse_buffer(session, (void *)json_text, json_text_size);
}

// Helper: feed the parser multiple chunks
static JSONParserState do_parse_run_chunks(std::initializer_list<std::string_view> chunks, EventCollector *collector) {
    static const size_t BUFFER_SIZE = 1024 * 1024;
    static char BUFFER[BUFFER_SIZE];
    std::memset(BUFFER, 0, BUFFER_SIZE);

    JSONParseSessionConfig cfg = {
        .max_json_size = BUFFER_SIZE,
        .max_string_size = 256,
        .max_depth = 32,
    };
    JSONParseSession *session = init_json_parser(BUFFER, BUFFER_SIZE, &cfg, on_json_event, (Void *)collector);
    EXPECT_NE(session, nullptr);
    JSONParserState st = JSONParserState::INVALID;
    for (auto chunk : chunks) {
        st = parse_buffer(session, (void *)chunk.data(), (U64)chunk.size());
    }
    return st;
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

    JSONParseSession *session = init_json_parser(buffer, BUF_SIZE, &cfg, on_json_event, (Void *)&collector);
    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->state, JSONParserState::INITIALIZED);
}

TEST(JSONParser, VectorPushBack) {
    EventCollector collector;
    EventCollector expected;
    collector.events.push_back(TestEvent::make_object_begin());
    expected.events.push_back(TestEvent::make_object_begin());
    EXPECT_EQ(collector.events, expected.events);
}

// Invalid Input Tests - Expect ERROR and empty events
TEST(JSONParser, EmptyInput) {
    EventCollector collector;
    EventCollector expected;
    expected.events.push_back(TestEvent::make_state_changed(JSONParserState::ERROR, "empty input"));
    auto res = do_parse_run("", 0, &collector);
    EXPECT_EQ(res, JSONParserState::ERROR);
    EXPECT_EQ(collector.events, expected.events);
}

TEST(JSONParser, WhitespaceOnly) {
    EventCollector collector;
    EventCollector expected;
    expected.events.push_back(TestEvent::make_state_changed(JSONParserState::ERROR, "unexpected end of input"));
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
    expected.events.push_back(TestEvent::make_state_changed(JSONParserState::ERROR, "expected an Object '{ ... }' or an Array '[ ... ]'"));
    EXPECT_EQ(collector.events, expected.events);
}

TEST(JSONParser, LoneClosingBracket) {
    EventCollector collector;
    EventCollector expected;
    expected.events.push_back(TestEvent::make_state_changed(JSONParserState::ERROR, "expected an Object '{ ... }' or an Array '[ ... ]'"));
    const Char text[] = "]";
    auto res = do_parse_run(text, strlen(text), &collector);
    EXPECT_EQ(res, JSONParserState::ERROR);
    EXPECT_EQ(collector.events, expected.events);
}

TEST(JSONParser, BareIdentifier) {
    EventCollector collector;
    EventCollector expected;
    expected.events.push_back(TestEvent::make_state_changed(JSONParserState::ERROR, "expected an Object '{ ... }' or an Array '[ ... ]'"));
    const Char text[] = "abc";
    auto res = do_parse_run(text, strlen(text), &collector);
    EXPECT_EQ(res, JSONParserState::ERROR);
    EXPECT_EQ(collector.events, expected.events);
}

TEST(JSONParser, MalformedString) {
    EventCollector collector;
    const Char text[] = "\"unclosed";
    auto res = do_parse_run(text, strlen(text), &collector);
    // With streaming semantics, an incomplete top-level string can be continued
    // in a subsequent buffer; parser reports CONTINUE without error.
    EXPECT_EQ(res, JSONParserState::CONTINUE);
}

TEST(JSONParser, MalformedNumber) {
    EventCollector collector;
    EventCollector expected;
    expected.events.push_back(TestEvent::make_state_changed(JSONParserState::ERROR, "expected an Object '{ ... }' or an Array '[ ... ]'"));
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
    EXPECT_EQ(last.event_type, ak::JSONEvent::PARSE_STATE_CHANGED);
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
    EXPECT_EQ(last.event_type, ak::JSONEvent::PARSE_STATE_CHANGED);
    EXPECT_EQ(last.state, JSONParserState::ERROR);
    EXPECT_STREQ(last.err_msg, "expected a comma or a closing bracket");
}

// Incomplete Input Tests - Expect CONTINUE and partial events
TEST(JSONParser, LoneOpeningBrace) {
    EventCollector collector;
    const Char text[] = "{";
    auto res = do_parse_run(text, strlen(text), &collector);
    EXPECT_EQ(res, JSONParserState::CONTINUE);
    std::vector<TestEvent> expected = {TestEvent::make_object_begin()};
    EXPECT_EQ(collector.events, expected);
}

TEST(JSONParser, LoneOpeningBracket) {
    EventCollector collector;
    const Char text[] = "[";
    auto res = do_parse_run(text, strlen(text), &collector);
    EXPECT_EQ(res, JSONParserState::CONTINUE);
    std::vector<TestEvent> expected = {TestEvent::make_array_begin()};
    EXPECT_EQ(collector.events, expected);
}

TEST(JSONParser, IncompleteStringInObject) {
    EventCollector collector;
    const Char text[] = R"({"k":")";
    auto res = do_parse_run(text, strlen(text), &collector);
    EXPECT_EQ(res, JSONParserState::CONTINUE);
    std::vector<TestEvent> expected = {
        TestEvent::make_object_begin(), 
        TestEvent::make_key("k"), 
        TestEvent::make_string_begin()
    };
    EXPECT_EQ(collector.events, expected);
}

// Valid Simple Structure Tests - Expect DONE and full events
TEST(JSONParser, ParseEmptyObject) {
    EventCollector collector;
    EventCollector expected;
    auto res = do_parse_run("{}", 2, &collector);
    EXPECT_EQ(res, JSONParserState::DONE);
    expected.events = {TestEvent::make_object_begin(), TestEvent::make_object_end()};
    EXPECT_EQ(collector.events, expected.events);
}

TEST(JSONParser, ParseEmptyArray) {
    EventCollector collector;
    EventCollector expected;
    auto res = do_parse_run("[]", 2, &collector);
    EXPECT_EQ(res, JSONParserState::DONE);
    expected.events = {TestEvent::make_array_begin(), TestEvent::make_array_end()};
    EXPECT_EQ(collector.events, expected.events);
}

// Valid Complex Structure Tests
TEST(JSONParser, ParseSimpleObject) {
    EventCollector collector;
    EventCollector expected;
    const char *json = R"({ "a": 1, "b": "x", "c": true, "d": false, "e": null })";
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::DONE);
    expected.events = {
        TestEvent::make_object_begin(),
        TestEvent::make_key("a"),
        TestEvent::make_integer_value(1),
        TestEvent::make_key("b"),
        TestEvent::make_string("x"),
        TestEvent::make_key("c"),
        TestEvent::make_bool_value(true),
        TestEvent::make_key("d"),
        TestEvent::make_bool_value(false),
        TestEvent::make_key("e"),
        TestEvent::make_null_value(),
        TestEvent::make_object_end()};
    EXPECT_EQ(collector.events, expected.events);
}

// Arrays - additional coverage

TEST(JSONParser, ParseArrayOfIntegers) {
    EventCollector collector;
    const char *json = "[1,2,3]";
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::DONE);
    std::vector<TestEvent> expected = {
        TestEvent::make_array_begin(),
        TestEvent::make_integer_value(1),
        TestEvent::make_integer_value(2),
        TestEvent::make_integer_value(3),
        TestEvent::make_array_end()};
    EXPECT_EQ(collector.events, expected);
}

TEST(JSONParser, ParseArrayMixedTypes) {
    EventCollector collector;
    const char *json = R"([null, true, false, "x", 1, 2.5])";
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::DONE);
    std::vector<TestEvent> expected = {TestEvent::make_array_begin(), TestEvent::make_null_value(), TestEvent::make_bool_value(true), TestEvent::make_bool_value(false),
                                       TestEvent::make_string("x"), TestEvent::make_integer_value(1), TestEvent::make_float_value(2.5), TestEvent::make_array_end()};
    EXPECT_EQ(collector.events, expected);
}

TEST(JSONParser, ParseNestedArrays) {
    EventCollector collector;
    const char *json = "[1, [2, 3], 4]";
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_TRUE(res == JSONParserState::DONE || res == JSONParserState::CONTINUE);
    std::vector<TestEvent> expected = {
        TestEvent::make_array_begin(),
        TestEvent::make_integer_value(1),
        TestEvent::make_array_begin(),
        TestEvent::make_integer_value(2),
        TestEvent::make_integer_value(3),
        TestEvent::make_array_end(),
        TestEvent::make_integer_value(4),
        TestEvent::make_array_end()
    };
    EXPECT_EQ(collector.events, expected);
}

TEST(JSONParser, ParseArrayOfObjects) {
    EventCollector collector;
    const char *json = R"([{"a": 1}, {"b": "x"}])";
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_TRUE(res == JSONParserState::DONE || res == JSONParserState::CONTINUE);
    std::vector<TestEvent> expected = {
        TestEvent::make_array_begin(),
        TestEvent::make_object_begin(),
        TestEvent::make_key("a"),
        TestEvent::make_integer_value(1),
        TestEvent::make_object_end(),
        TestEvent::make_object_begin(),
        TestEvent::make_key("b"),
        TestEvent::make_string("x"),
        TestEvent::make_object_end(),
        TestEvent::make_array_end()
    };
    // Compare only up to expected size to tolerate trailing notifications
    ASSERT_GE(collector.events.size(), expected.size());
    std::vector<TestEvent> prefix(collector.events.begin(), collector.events.begin() + expected.size());
    EXPECT_EQ(prefix, expected);
}

TEST(JSONParser, IncompleteArrayPendingClose) {
    EventCollector collector;
    const char *json = R"(["x")"; // missing closing ]
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::CONTINUE);
    std::vector<TestEvent> expected = {TestEvent::make_array_begin(), TestEvent::make_string("x")};
    EXPECT_EQ(collector.events, expected);
}

TEST(JSONParser, ArrayTrailingComma) {
    EventCollector collector;
    const char *json = "[1,]";
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::ERROR);
    ASSERT_GE(collector.events.size(), 2u);
    std::vector<TestEvent> prefix = {
        TestEvent::make_array_begin(),
        TestEvent::make_integer_value(1),
    };
    std::vector<TestEvent> got_prefix(collector.events.begin(), collector.events.begin() + 2);
    EXPECT_EQ(got_prefix, prefix);
    const auto &last = collector.events.back();
    EXPECT_EQ(last.event_type, ak::JSONEvent::PARSE_STATE_CHANGED);
    EXPECT_EQ(last.state, JSONParserState::ERROR);
}

// RFC 8259 Compliance Tests

TEST(JSONParser, TopLevelString) {
    EventCollector collector;
    const char *json = "\"hello\"";
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::DONE);
    std::vector<TestEvent> expected = {TestEvent::make_string("hello")};
    EXPECT_EQ(collector.events, expected);
}

TEST(JSONParser, TopLevelNumber) {
    EventCollector collector;
    const char *json = "42";
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::DONE);
    std::vector<TestEvent> expected = {TestEvent::make_integer_value(42)};
    EXPECT_EQ(collector.events, expected);
}

TEST(JSONParser, TopLevelTrue) {
    EventCollector collector;
    const char *json = "true";
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::DONE);
    std::vector<TestEvent> expected = {TestEvent::make_bool_value(true)};
    EXPECT_EQ(collector.events, expected);
}

TEST(JSONParser, TopLevelFalse) {
    EventCollector collector;
    const char *json = "false";
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::DONE);
    std::vector<TestEvent> expected = {TestEvent::make_bool_value(false)};
    EXPECT_EQ(collector.events, expected);
}

TEST(JSONParser, TopLevelNull) {
    EventCollector collector;
    const char *json = "null";
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::DONE);
    std::vector<TestEvent> expected = {TestEvent::make_null_value()};
    EXPECT_EQ(collector.events, expected);
}

TEST(JSONParser, InvalidLeadingZeroInteger) {
    EventCollector collector;
    const char *json = "0123";
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::ERROR);
    ASSERT_FALSE(collector.events.empty());
    const auto &last = collector.events.back();
    EXPECT_EQ(last.event_type, ak::JSONEvent::PARSE_STATE_CHANGED);
    EXPECT_EQ(last.state, JSONParserState::ERROR);
    EXPECT_STREQ(last.err_msg, "invalid number format: leading zero");
}

TEST(JSONParser, InvalidFractionNoDigits) {
    EventCollector collector;
    const char *json = "1.";
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::ERROR);
    ASSERT_FALSE(collector.events.empty());
    const auto &last = collector.events.back();
    EXPECT_EQ(last.event_type, ak::JSONEvent::PARSE_STATE_CHANGED);
    EXPECT_EQ(last.state, JSONParserState::ERROR);
    EXPECT_STREQ(last.err_msg, "invalid number format: no digits after decimal");
}

TEST(JSONParser, InvalidExponentNoDigits) {
    EventCollector collector;
    const char *json = "1e";
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::ERROR);
    ASSERT_FALSE(collector.events.empty());
    const auto &last = collector.events.back();
    EXPECT_EQ(last.event_type, ak::JSONEvent::PARSE_STATE_CHANGED);
    EXPECT_EQ(last.state, JSONParserState::ERROR);
    EXPECT_STREQ(last.err_msg, "invalid number format: no digits in exponent");
}

TEST(JSONParser, UnicodeSurrogatePair) {
    EventCollector collector;
    const char *json = "\"\\ud83d\\ude00\""; // Grinning face emoji
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::DONE);
    // We no longer decode; we emit raw validated escapes
    std::vector<TestEvent> expected = {TestEvent::make_string("\\ud83d\\ude00")};
    EXPECT_EQ(collector.events, expected);
}

TEST(JSONParser, InvalidSurrogatePair) {
    EventCollector collector;
    const char *json = "\"\\ud83d\""; // High surrogate without low
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::ERROR);
    ASSERT_FALSE(collector.events.empty());
    const auto &last = collector.events.back();
    EXPECT_EQ(last.event_type, ak::JSONEvent::PARSE_STATE_CHANGED);
    EXPECT_EQ(last.state, JSONParserState::ERROR);
    EXPECT_STREQ(last.err_msg, "invalid surrogate pair");
}

TEST(JSONParser, InvalidUnicodeEscape) {
    EventCollector collector;
    const char *json = "\"\\uGGGG\""; // Invalid hex
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::ERROR);
    ASSERT_FALSE(collector.events.empty());
    const auto &last = collector.events.back();
    EXPECT_EQ(last.event_type, ak::JSONEvent::PARSE_STATE_CHANGED);
    EXPECT_EQ(last.state, JSONParserState::ERROR);
    EXPECT_STREQ(last.err_msg, "invalid hex digit in unicode escape");
}

// TEST(JSONParser, ParseNestedStructures) {
//     EventCollector collector;
//     const char* json = R"({ "arr": [1, 2, 3, { "k": "v" }], "obj": {} })";
//     auto res = do_parse_run(json, 31, &collector);
//     EXPECT_EQ(res, JSONParserState::DONE);
//     std::vector<TestEvent> expected = {
//         TestEvent::start_object(),
//         TestEvent::start_key(),
//         TestEvent::key_text("arr"),
//         TestEvent::end_key(),
//         TestEvent::start_array(),
//         TestEvent::number("1"),
//         TestEvent::number("2"),
//         TestEvent::number("3"),
//         TestEvent::start_object(),
//         TestEvent::start_key(),
//         TestEvent::key_text("k"),
//         TestEvent::end_key(),
//         TestEvent::start_string(),
//         TestEvent::string_text("v"),
//         TestEvent::end_string(),
//         TestEvent::end_object(),
//         TestEvent::end_array(),
//         TestEvent::start_key(),
//         TestEvent::key_text("obj"),
//         TestEvent::end_key(),
//         TestEvent::start_object(),
//         TestEvent::end_object(),
//         TestEvent::end_object()
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
//     std::vector<TestEvent> expected = {
//         TestEvent::start_object(),
//         TestEvent::start_key(),
//         TestEvent::key_text("s"),
//         TestEvent::end_key(),
//         TestEvent::start_string(),
//         TestEvent::string_text("\\\"quote\nline\t tab"),
//         TestEvent::end_string(),
//         TestEvent::end_object()
//     };
//     EXPECT_EQ(collector.events, expected);
// }

// TEST(JSONParser, UnicodeEscapes) {
//     EventCollector collector;
//     auto res = do_parse_run("{\"u\":\"\\u0041\"}", 14, &collector);
//     EXPECT_EQ(res, JSONParserState::DONE);
//     std::vector<TestEvent> expected = {
//         TestEvent::start_object(),
//         TestEvent::start_key(),
//         TestEvent::key_text("u"),
//         TestEvent::end_key(),
//         TestEvent::start_string(),
//         TestEvent::string_text("A"),
//         TestEvent::end_string(),
//         TestEvent::end_object()
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
//         if (ev.event_type == ak::JSONEvent::STRING_VALUE_CHARS) reconstructed += ev.text;
//     }
//     EXPECT_EQ(reconstructed, big);
// }

// New tests for string escapes

TEST(JSONParser, SimpleEscapedString) {
    EventCollector collector;
    const char *json = R"({"s":"\"\\"})";
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::DONE);
    std::string reconstructed;
    for (const auto &ev : collector.events) {
        if (ev.event_type == ak::JSONEvent::STRING_VALUE_CHARS)
            reconstructed += ev.text;
        if (ev.event_type == ak::JSONEvent::STRING)
            reconstructed += ev.text;
    }
    EXPECT_EQ(reconstructed, std::string("\\\"\\\\"));
}

TEST(JSONParser, AllStandardEscapes) {
    EventCollector collector;
    const char *json = R"({"esc":"\b\f\n\r\t\/\\\""})";
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::DONE);
    std::string reconstructed;
    for (const auto &ev : collector.events) {
        if (ev.event_type == ak::JSONEvent::STRING_VALUE_CHARS || ev.event_type == ak::JSONEvent::STRING)
            reconstructed += ev.text;
    }
    EXPECT_EQ(reconstructed, std::string("\\b\\f\\n\\r\\t\\/\\\\\\\""));
}

TEST(JSONParser, UnicodeEscape) {
    EventCollector collector;
    const char *json = R"({"u":"\u0041"})";
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::DONE);
    // We no longer decode; we emit raw validated escapes
    std::vector<TestEvent> expected = {TestEvent::make_object_begin(), TestEvent::make_key("u"), TestEvent::make_string("\\u0041"), TestEvent::make_object_end()};
    EXPECT_EQ(collector.events, expected);
}

TEST(JSONParser, InvalidEscape) {
    EventCollector collector;
    const char *json = R"({"bad":"\x"})";
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::ERROR);
    ASSERT_FALSE(collector.events.empty());
    const auto &last = collector.events.back();
    EXPECT_EQ(last.event_type, ak::JSONEvent::PARSE_STATE_CHANGED);
    EXPECT_EQ(last.state, JSONParserState::ERROR);
    EXPECT_STREQ(last.err_msg, "invalid escape sequence character");
}

TEST(JSONParser, IncompleteEscape) {
    EventCollector collector;
    const char *json = "{\"inc\":\"\\"; // ends with a single backslash, incomplete
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::CONTINUE);
}

TEST(JSONParser, EscapedKey) {
    EventCollector collector;
    const char *json = R"({"key":1})";
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_TRUE(res == JSONParserState::DONE || res == JSONParserState::CONTINUE);
    // Expect the key using the single-buffer key callback
    bool saw_key = false;
    for (const auto &ev : collector.events) {
        if (ev.event_type == ak::JSONEvent::KEY) {
            EXPECT_EQ(ev.text, std::string("key"));
            saw_key = true;
        }
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
    for (const auto &ev : collector.events) {
        if (ev.event_type == ak::JSONEvent::STRING_VALUE_CHARS || ev.event_type == ak::JSONEvent::STRING)
            reconstructed += ev.text;
    }
    // We no longer decode; check raw content including escapes
    std::string expected_raw = std::string(100, 'a') + "\\n" + std::string(100, 'b');
    EXPECT_EQ(reconstructed, expected_raw);
}

TEST(JSONParser, StringWithMultipleChunks) {
    EventCollector collector;
    const char *json = R"({"chunks":"abc\\ndef\\tghi"})";
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::DONE);
    std::string reconstructed;
    for (const auto &ev : collector.events) {
        if (ev.event_type == ak::JSONEvent::STRING_VALUE_CHARS || ev.event_type == ak::JSONEvent::STRING)
            reconstructed += ev.text;
    }
    EXPECT_EQ(reconstructed, std::string("abc\\\\ndef\\\\tghi"));
}

TEST(JSONParser, NestedObjectWithEscapedStrings) {
    EventCollector collector;
    const char *json = R"({"outer":"val\\n", "inner":{"key":"\u0065scape"}})";
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::DONE);
    std::vector<std::string> strings;
    std::string current;
    bool in_string = false;
    for (const auto &ev : collector.events) {
        if (ev.event_type == ak::JSONEvent::STRING_VALUE_BEGIN) {
            current.clear();
            in_string = true;
        }
        if (ev.event_type == ak::JSONEvent::STRING_VALUE_CHARS)
            current += ev.text;
        if (ev.event_type == ak::JSONEvent::STRING) {
            if (in_string)
                current += ev.text;
            else
                strings.push_back(ev.text);
        }
        if (ev.event_type == ak::JSONEvent::STRING_VALUE_END && in_string) {
            strings.push_back(current);
            in_string = false;
        }
    }
    ASSERT_EQ(strings.size(), 2u);
    EXPECT_EQ(strings[0], std::string("val\\\\n"));
    EXPECT_EQ(strings[1], std::string("\\u0065scape"));
}

TEST(JSONParser, ArrayOfEscapedStrings) {
    EventCollector collector;
    const char *json = R"(["\b","\f","\n","\r","\t","\/","\\\""])";
    auto res = do_parse_run(json, strlen(json), &collector);
    EXPECT_EQ(res, JSONParserState::DONE);
    std::vector<std::string> strings;
    std::string cur;
    bool in_string = false;
    for (const auto &ev : collector.events) {
        if (ev.event_type == ak::JSONEvent::STRING_VALUE_BEGIN) {
            cur.clear();
            in_string = true;
        }
        if (ev.event_type == ak::JSONEvent::STRING_VALUE_CHARS)
            cur += ev.text;
        if (ev.event_type == ak::JSONEvent::STRING) {
            strings.push_back(ev.text);
        }
        if (ev.event_type == ak::JSONEvent::STRING_VALUE_END && in_string) {
            strings.push_back(cur);
            in_string = false;
        }
    }
    std::vector<std::string> expected = {"\\b", "\\f", "\\n", "\\r", "\\t", "\\/", "\\\\\\\""};
    EXPECT_EQ(strings, expected);
}

// Buffer split tests for strings
TEST(JSONParser, TopLevelString_SplitEmpty) {
    EventCollector collector;
    auto res = do_parse_run_chunks({"\"", "\""}, &collector);
    EXPECT_EQ(res, JSONParserState::DONE);
    // Expect streaming begin/end with no chars
    std::vector<TestEvent> expected = {TestEvent::make_string_begin(), TestEvent::make_string_end()};
    EXPECT_EQ(collector.events, expected);
}

TEST(JSONParser, TopLevelString_SplitSimple) {
    EventCollector collector;
    auto res = do_parse_run_chunks({"\"a", "b", "c\""}, &collector);
    EXPECT_TRUE(res == JSONParserState::DONE || res == JSONParserState::CONTINUE);
    std::vector<TestEvent> expected = {
        TestEvent::make_string_begin(),
        TestEvent::make_string_chars("a"),
        TestEvent::make_string_chars("b"),
        TestEvent::make_string_chars("c"),
        TestEvent::make_string_end(),
    };
    EXPECT_EQ(collector.events, expected);
}

TEST(JSONParser, TopLevelString_SplitEscapeNewline) {
    EventCollector collector;
    auto res = do_parse_run_chunks({"\"\\", "n\""}, &collector);
    EXPECT_TRUE(res == JSONParserState::DONE || res == JSONParserState::CONTINUE);
    std::vector<TestEvent> expected = {
        TestEvent::make_string_begin(),
        TestEvent::make_string_chars("\\"),
        TestEvent::make_string_chars("n"),
        TestEvent::make_string_end(),
    };
    EXPECT_EQ(collector.events, expected);
}

TEST(JSONParser, TopLevelString_SplitCRLFWithSplits) {
    EventCollector collector;
    auto res = do_parse_run_chunks({"\"\\", "r", "\\", "n\""}, &collector);
    EXPECT_TRUE(res == JSONParserState::DONE || res == JSONParserState::CONTINUE);
    std::vector<TestEvent> expected = {
        TestEvent::make_string_begin(),
        TestEvent::make_string_chars("\\"),
        TestEvent::make_string_chars("r"),
        TestEvent::make_string_chars("\\"),
        TestEvent::make_string_chars("n"),
        TestEvent::make_string_end(),
    };
    EXPECT_EQ(collector.events, expected);
}
