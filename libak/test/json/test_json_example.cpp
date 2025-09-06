#include <gtest/gtest.h>

#include "ak.hpp" // IWYU pragma: keep

int on_parse_event(AkJSONParser* session, AkJSONEvent event, const AkJSONEventData* data, AkU64 more) noexcept {
    (void) session;
    (void) data;
    (void) more;
    switch (event) {
        case AkJSONEvent::OBJECT_BEGIN:
        {
            std::print("OBJECT_BEGIN\n");
            return 0;
        }
        case AkJSONEvent::OBJECT_END:
        {
            std::print("OBJECT_END\n");
            return 0;
        }
        case AkJSONEvent::ARRAY_END:
        {
            std::print("OBJECT_END\n");
            return 0; 
        }
        case AkJSONEvent::NULL_VALUE:
        {
            std::print("NULL_VALUE\n");
            return 0;
        }
        case AkJSONEvent::ATTR_KEY:
        {
            std::print("ATTR_KEY '{}'\n", std::string_view(data->string_data.str, data->string_data.len));
            return 0; 
        }
        case AkJSONEvent::STRING_VALUE:
        {
            std::print("STRING_VALUE '{}'\n", std::string_view(data->string_data.str, data->string_data.len));
            return 0;
        }
        case AkJSONEvent::INT_VALUE:
        {
            std::print("INT_VALUE {}\n", data->int_value);
            return 0;
        }
        case AkJSONEvent::FLOAT_VALUE:
        {
            std::print("FLOAT_VALUE {}\n", data->float_value);
            return 0;
        }
        case AkJSONEvent::BOOL_VALUE:
        {
            std::print("BOOL_VALUE {}\n", data->bool_value);
            return 0;
        }
        case AkJSONEvent::PARSE_STATE_CHANGED:
        {
            std::print("PARSE_STATE_CHANGED '{}'\n", (AkU32)data->state_data.state);
            return 0;
        }
        case AkJSONEvent::PARSE_EOF:
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
    const char json[] = R"({"name": "John", "age": 30})";
    const AkU64 json_size = sizeof(json);

    AkJSONParserConfig cfg = { };
    cfg.max_depth = 32;
    cfg.max_string_size = 2048;
    cfg.max_json_size = 1024 * 1024;
    
    auto* session = ak_init_json_parser(buffer, sizeof(buffer), &cfg, on_parse_event, nullptr);
    ASSERT_NE(session, nullptr);
    ASSERT_EQ(session->state, AkJSONParserState::INITIALIZED);


    AkJSONParserState state;
    state = ak_run_json_parser(session, (void*)json, json_size);
    ASSERT_EQ(state, AkJSONParserState::DONE);
    state = ak_eof_json_parser(session);
    ASSERT_EQ(state, AkJSONParserState::DONE);
}
