#include <gtest/gtest.h>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <ak.hpp>

using namespace ak;
namespace fs = std::filesystem;

struct SerializedSink {
    std::vector<std::string> lines;
    U32 last_err_code = 0;
};

static void on_json_event(JSONParseSession *session, ak::JSONEvent event, const JSONEventData *data) noexcept {
    auto *sink = static_cast<SerializedSink *>(session->user_data);
    if (!sink) return;
    switch (event) {
        case ak::JSONEvent::OBJECT_BEGIN: sink->lines.emplace_back("BEGIN_OBJECT"); break;
        case ak::JSONEvent::OBJECT_END: sink->lines.emplace_back("END_OBJECT"); break;
        case ak::JSONEvent::ARRAY_BEGIN: sink->lines.emplace_back("BEGIN_ARRAY"); break;
        case ak::JSONEvent::ARRAY_END: sink->lines.emplace_back("END_ARRAY"); break;
        case ak::JSONEvent::ATTR_KEY_BEGIN: sink->lines.emplace_back("KEY_BEGIN"); break;
        case ak::JSONEvent::ATTR_KEY_END: sink->lines.emplace_back("KEY_END"); break;
        case ak::JSONEvent::ATTR_KEY_CHARS:
            if (data) sink->lines.emplace_back(std::string("KEY_CHARS ") + std::string(data->string_data.str, data->string_data.len));
            break;
        case ak::JSONEvent::KEY:
            if (data) sink->lines.emplace_back(std::string("KEY ") + std::string(data->string_data.str, data->string_data.len));
            break;
        case ak::JSONEvent::NULL_VALUE: sink->lines.emplace_back("NULL"); break;
        case ak::JSONEvent::BOOL_VALUE:
            if (data) sink->lines.emplace_back(std::string("BOOL ") + (data->bool_value ? "true" : "false"));
            break;
        case ak::JSONEvent::INT_VALUE:
            if (data) sink->lines.emplace_back(std::string("INT ") + std::to_string((long long)data->int_value));
            break;
        case ak::JSONEvent::FLOAT_VALUE: {
            if (data) { std::ostringstream os; os.setf(std::ios::fmtflags(0), std::ios::floatfield); os.precision(17); os << data->float_value; sink->lines.emplace_back(std::string("FLOAT ") + os.str()); }
            break; }
        case ak::JSONEvent::STRING_VALUE_BEGIN: sink->lines.emplace_back("STRING_BEGIN"); break;
        case ak::JSONEvent::STRING_VALUE_END: sink->lines.emplace_back("STRING_END"); break;
        case ak::JSONEvent::STRING_VALUE_CHARS:
            if (data) sink->lines.emplace_back(std::string("STRING_CHARS ") + std::string(data->string_data.str, data->string_data.len));
            break;
        case ak::JSONEvent::STRING:
            if (data) sink->lines.emplace_back(std::string("STRING ") + std::string(data->string_data.str, data->string_data.len));
            break;
        case ak::JSONEvent::PARSE_STATE_CHANGED:
            if (data) {
                sink->last_err_code = data->state_data.err_code;
                if (data->state_data.state == JSONParserState::ERROR) {
                    sink->lines.emplace_back(std::string("ERROR ") + std::to_string((unsigned long long)data->state_data.err_code));
                }
            }
            break;
        case ak::JSONEvent::ATTR_BEGIN:
        case ak::JSONEvent::ATTR_END:
            break;
    }
}

static JSONParserState parse_json_chunks(const std::vector<std::string> &chunks, SerializedSink &sink, U32 &out_err_code) {
    static constexpr size_t BUFFER_SIZE = 1024 * 1024;
    static char BUFFER[BUFFER_SIZE];
    std::memset(BUFFER, 0, BUFFER_SIZE);
    JSONParseSessionConfig cfg = { .max_json_size = BUFFER_SIZE, .max_string_size = 256, .max_depth = 32 };
    JSONParseSession *session = init_json_parser(BUFFER, BUFFER_SIZE, &cfg, on_json_event, (Void *)&sink);
    EXPECT_NE(session, nullptr);
    JSONParserState st = JSONParserState::INVALID;
    for (const auto &chunk : chunks) {
        st = parse_buffer(session, (void *)chunk.data(), (U64)chunk.size());
    }
    out_err_code = (st == JSONParserState::ERROR) ? sink.last_err_code : 0;
    return st;
}

static std::string serialize_out(JSONParserState st, const SerializedSink &sink, U32 err_code) {
    std::ostringstream os;
    switch (st) {
        case JSONParserState::DONE: os << "result=DONE\n"; break;
        case JSONParserState::CONTINUE: os << "result=CONTINUE\n"; break;
        case JSONParserState::ERROR: os << "result=ERROR\n"; break;
        case JSONParserState::INITIALIZED: os << "result=INITIALIZED\n"; break;
        default: os << "result=INVALID\n"; break;
    }
    if (st == JSONParserState::ERROR) {
        os << "value=" << err_code << "\n";
    }
    os << "---\n";
    os << "EVENT\n";
    for (const auto &ln : sink.lines) os << ln << "\n";
    return os.str();
}

static bool read_input_case(const fs::path &p, std::vector<std::pair<std::string,std::string>> &kv, std::vector<std::string> &chunks) {
    std::ifstream f(p);
    if (!f.is_open()) return false;
    std::string line; bool in_json = false; std::ostringstream current;
    while (std::getline(f, line)) {
        if (!in_json) {
            if (line.rfind("----------", 0) == 0) { in_json = true; continue; }
            auto eq = line.find('=');
            if (eq != std::string::npos) kv.emplace_back(line.substr(0, eq), line.substr(eq + 1));
        } else {
            if (line == "---") {
                std::string part = current.str();
                if (!part.empty() && part.back() == '\n') part.pop_back();
                chunks.push_back(std::move(part));
                current.str(""); current.clear();
            } else {
                current << line << '\n';
            }
        }
    }
    std::string last = current.str();
    if (!last.empty() && last.back() == '\n') last.pop_back();
    if (!last.empty() || chunks.empty()) chunks.push_back(std::move(last));
    return true;
}

struct JSONCaseParam { std::string name; fs::path input; fs::path expected; };

static std::vector<JSONCaseParam> discover_cases(const fs::path &data_root) {
    std::vector<JSONCaseParam> out;
    if (!fs::exists(data_root)) return out;
    for (auto &entry : fs::directory_iterator(data_root)) {
        if (!entry.is_regular_file()) continue;
        fs::path in_path = entry.path();
        std::string name = in_path.filename().string();
        // Skip expected files (those ending with _exp.txt)
        if (name.size() >= 8 && name.rfind("_exp.txt") == name.size() - 8) continue;
        fs::path expected = data_root / (in_path.stem().string() + "_exp.txt");
        if (fs::exists(expected) && fs::is_regular_file(expected)) {
            out.push_back(JSONCaseParam{name, in_path, expected});
        }
    }
    return out;
}

class JSONParser : public ::testing::TestWithParam<JSONCaseParam> {};

TEST_P(JSONParser, Case) {
    const auto param = GetParam();
    std::vector<std::pair<std::string,std::string>> kv; std::vector<std::string> chunks;
    ASSERT_TRUE(read_input_case(param.input, kv, chunks));

    SerializedSink sink; 
    U32 err_code = 0; 
    JSONParserState st = parse_json_chunks(chunks, sink, err_code);
    std::string actual = serialize_out(st, sink, err_code);

    const char *env_out = std::getenv("AK_TEST_OUTPUT_DIR");
    fs::path out_dir = env_out ? fs::path(env_out) : fs::path("build/test_output/json");
    fs::create_directories(out_dir / param.name);
    fs::path out_file = out_dir / param.name / "output.txt";
    std::ofstream ofs(out_file); 
    ofs << actual; 
    ofs.close();

    std::ifstream exp_f(param.expected); 
    ASSERT_TRUE(exp_f.is_open());
    std::ostringstream exp_ss; 
    exp_ss << exp_f.rdbuf(); 
    std::string expected = exp_ss.str();
    EXPECT_EQ(actual, expected) << "Case '" << param.name << "' mismatched. See " << out_file.string();
}

static std::vector<JSONCaseParam> load_params() {
    const char *env_data = std::getenv("AK_TEST_DATA_DIR");
    fs::path data_root = env_data ? fs::path(env_data) : fs::path("libak/test/json/data");
    return discover_cases(data_root);
}

struct NamePrinter {
    std::string operator()(const ::testing::TestParamInfo<JSONCaseParam>& info) const {
        // Use the full filename with extension stripped for uniqueness
        std::string name = info.param.name;
        // Remove .txt extension
        if (name.size() > 4 && name.substr(name.size() - 4) == ".txt") {
            name = name.substr(0, name.size() - 4);
        }
        // Replace non-alphanumeric characters with their hex values for uniqueness
        std::string sanitized;
        for (char c : name) {
            if (std::isalnum(c) || c == '_') {
                sanitized += c;
            } else {
                // Replace special characters with hex codes to ensure uniqueness
                char hex[8];
                std::snprintf(hex, sizeof(hex), "_%02x", (unsigned char)c);
                sanitized += hex;
            }
        }
        return sanitized;
    }
};

INSTANTIATE_TEST_SUITE_P(JSONParser, JSONParser, ::testing::ValuesIn(load_params()), NamePrinter());



