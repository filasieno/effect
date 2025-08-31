#include <gtest/gtest.h>

#include "ak.hpp" // IWYU pragma: keep

using namespace ak;

I32 on_parse_event(JSONParseSession* session, JSONEvent event, const JSONEventData* data, U64 more) noexcept {
    (void) session;
    (void) data;
    (void) more;
    switch (event) {
        case JSONEvent::OBJECT_BEGIN:
        {
            std::print("OBJECT_BEGIN\n");
            return 0;
        }
        case JSONEvent::OBJECT_END:
        {
            std::print("OBJECT_END\n");
            return 0;
        }
        case JSONEvent::ARRAY_END:
        {
            std::print("OBJECT_END\n");
            return 0; 
        }
        case JSONEvent::NULL_VALUE:
        {
            std::print("NULL_VALUE\n");
            return 0;
        }
        case JSONEvent::ATTR_KEY:
        {
            std::print("ATTR_KEY '{}'\n", std::string_view(data->string_data.str, data->string_data.len));
            return 0; 
        }
        case JSONEvent::STRING_VALUE:
        {
            std::print("STRING_VALUE '{}'\n", std::string_view(data->string_data.str, data->string_data.len));
            return 0;
        }
        case JSONEvent::INT_VALUE:
        {
            std::print("INT_VALUE {}\n", data->int_value);
            return 0;
        }
        case JSONEvent::FLOAT_VALUE:
        {
            std::print("FLOAT_VALUE {}\n", data->float_value);
            return 0;
        }
        case JSONEvent::BOOL_VALUE:
        {
            std::print("BOOL_VALUE {}\n", data->bool_value);
            return 0;
        }
        case JSONEvent::PARSE_STATE_CHANGED:
        {
            std::print("PARSE_STATE_CHANGED '{}'\n", (U32)data->state_data.state);
            return 0;
        }
        case JSONEvent::PARSE_EOF:
        {
            std::print("PARSE_EOF\n");
            return 0;
        }
        default: 
            return 0;        
    }
};
        
char buffer[1024 * 1024];

TEST(JSONParserTest, ReaderWriterHandshake) {
    const Char json[] = R"({"name": "John", "age": 30})";
    const U64 json_size = sizeof(json);

    JSONParseSessionConfig cfg = { };
    cfg.max_depth = 32;
    cfg.max_string_size = 2048;
    cfg.max_json_size = 1024 * 1024;
    
    auto* session = ak::init_json_parser(buffer, sizeof(buffer), &cfg, on_parse_event, nullptr);
    ASSERT_NE(session, nullptr);
    ASSERT_EQ(session->state, JSONParserState::INITIALIZED);


    JSONParserState state;
    state = ak::run_json_parser(session, (Void*)json, json_size);
    ASSERT_EQ(state, JSONParserState::DONE);
    state = ak::eof_json_parser(session);
    ASSERT_EQ(state, JSONParserState::DONE);
}
