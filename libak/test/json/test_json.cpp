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
    // For multi-buffer tests, store intermediate results
    std::vector<std::vector<std::string>> buffer_results;
    std::vector<JSONParserState> buffer_states;
    std::vector<U32> buffer_error_codes;
};

static int on_json_event(JSONParseSession *session, ak::JSONEvent event, const JSONEventData *data, U64 more) noexcept {
    auto *sink = static_cast<SerializedSink *>(session->user_data);
    if (!sink) return 0;
    switch (event) {
        case ak::JSONEvent::OBJECT_BEGIN: sink->lines.emplace_back("BEGIN_OBJECT"); break;
        case ak::JSONEvent::OBJECT_END: sink->lines.emplace_back("END_OBJECT"); break;
        case ak::JSONEvent::ARRAY_BEGIN: sink->lines.emplace_back("BEGIN_ARRAY"); break;
        case ak::JSONEvent::ARRAY_END: sink->lines.emplace_back("END_ARRAY"); break;
        case ak::JSONEvent::ATTR_KEY:
            if (data) sink->lines.emplace_back(std::string("ATTR_KEY \"") + std::string(data->string_data.str, data->string_data.len) + "\" more=" + (more ? "1" : "0"));
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
        case ak::JSONEvent::STRING_VALUE:
            if (data) sink->lines.emplace_back(std::string("STRING_VALUE \"") + std::string(data->string_data.str, data->string_data.len) + "\" more=" + (more ? "1" : "0"));
            break;
        case ak::JSONEvent::PARSE_STATE_CHANGED:
            if (data) {
                sink->last_err_code = data->state_data.err_code;
                switch (data->state_data.state) {
                    case JSONParserState::INITIALIZED:
                        sink->lines.emplace_back("STATE_CHANGED_EVENT: STATE_INITIALIZED");
                        break;
                    case JSONParserState::CONTINUE:
                        sink->lines.emplace_back("STATE_CHANGED_EVENT: STATE_CONTINUE");
                        break;
                    case JSONParserState::DONE:
                        sink->lines.emplace_back("STATE_CHANGED_EVENT: STATE_DONE");
                        break;
                    case JSONParserState::ERROR:
                        sink->lines.emplace_back(std::string("STATE_CHANGED_EVENT: STATE_ERROR ") + std::to_string((unsigned long long)data->state_data.err_code));
                        break;
                    default:
                        sink->lines.emplace_back("STATE_CHANGED_EVENT: STATE_INVALID");
                        break;
                }
            }
            break;
        case ak::JSONEvent::PARSE_EOF:
            sink->lines.emplace_back("PARSE_EOF_EVENT");
            break;
    }
    return 0;
}

static JSONParserState parse_json_chunks(const std::vector<std::pair<std::string,std::string>> &kv,
                                         const std::vector<std::string> &chunks,
                                         SerializedSink &sink, U32 &out_err_code, std::ostream &log_stream) {
    // Defaults; may be overridden by key/values in the test input header
    JSONParseSessionConfig cfg = { };
    // Apply key/value configuration
    for (const auto &p : kv) {
        if (p.first == "max_depth") {
            unsigned long long v = std::strtoull(p.second.c_str(), nullptr, 10);
            if (v > 0 && v <= std::numeric_limits<U32>::max()) cfg.max_depth = (U32)v;
        } else if (p.first == "max_string_size") {
            unsigned long long v = std::strtoull(p.second.c_str(), nullptr, 10);
            if (v > 0) cfg.max_string_size = (U64)v;
        } else if (p.first == "max_json_size") {
            unsigned long long v = std::strtoull(p.second.c_str(), nullptr, 10);
            if (v > 0) cfg.max_json_size = (U64)v;
        }
    }

    // Determine required parser buffer size and allocate dynamically
    U64 required_size = get_required_parse_session_buffer_size(&cfg);
    log_stream << "INFO: Required parser buffer size: " << required_size << " bytes\n";
    void *parser_mem = std::malloc((size_t)required_size);
    if (!parser_mem) {
        log_stream << "ERROR: Failed to allocate parser buffer of size " << required_size << "\n";
        out_err_code = (U32)JSONErrorCode::FATAL_STACK_OOB; // generic internal error for OOM in tests
        return JSONParserState::ERROR;
    }
    std::memset(parser_mem, 0, (size_t)required_size);
    JSONParseSession *session = init_json_parser(parser_mem, required_size, &cfg, on_json_event, (Void *)&sink);
    if (!session) {
        log_stream << "ERROR: Failed to initialize JSON parser session\n";
        std::free(parser_mem);
        return JSONParserState::ERROR;
    }
    log_stream << "INFO: JSON parser session initialized successfully\n";
    JSONParserState st = JSONParserState::INVALID;

    for (size_t i = 0; i < chunks.size(); ++i) {
        log_stream << "INFO: Processing chunk " << (i + 1) << "/" << chunks.size() << " (size: " << chunks[i].size() << " bytes)\n";

        // Capture lines before this chunk for intermediate results
        size_t chunk_start_lines = sink.lines.size();

        st = run_json_parser(session, (void *)chunks[i].data(), (U64)chunks[i].size());
        log_stream << "INFO: Chunk " << (i + 1) << " processing result: " << (st == JSONParserState::DONE ? "DONE" :
                                                                               st == JSONParserState::CONTINUE ? "CONTINUE" :
                                                                               st == JSONParserState::ERROR ? "ERROR" : "INVALID") << "\n";

        // Store intermediate results for this buffer
        std::vector<std::string> chunk_lines(sink.lines.begin() + chunk_start_lines, sink.lines.end());
        sink.buffer_results.push_back(chunk_lines);
        sink.buffer_states.push_back(st);
        sink.buffer_error_codes.push_back((st == JSONParserState::ERROR) ? sink.last_err_code : 0);

        if (st == JSONParserState::ERROR) break;
    }

    // Always call stop_json_parser to signal end of input
    if (st == JSONParserState::CONTINUE) {
        log_stream << "INFO: Calling stop_json_parser to signal end of input\n";

        // Capture lines before stop_json_parser for intermediate results
        size_t lines_before_stop = sink.lines.size();

        st = eof_json_parser(session);
        log_stream << "INFO: stop_json_parser result: " << (st == JSONParserState::DONE ? "DONE" :
                                                             st == JSONParserState::CONTINUE ? "CONTINUE" :
                                                             st == JSONParserState::ERROR ? "ERROR" : "INVALID") << "\n";

        // Store results from stop_json_parser as a separate "buffer"
        std::vector<std::string> stop_lines(sink.lines.begin() + lines_before_stop, sink.lines.end());
        sink.buffer_results.push_back(stop_lines);
        sink.buffer_states.push_back(st);
        sink.buffer_error_codes.push_back((st == JSONParserState::ERROR) ? sink.last_err_code : 0);
    }

    // If we have multiple buffers and the first buffer doesn't have STATE_INITIALIZED,
    // add it to the first buffer
    if (!sink.buffer_results.empty() && sink.buffer_results[0].size() > 0) {
        // Check if the first event in the first buffer is not STATE_INITIALIZED
        if (sink.buffer_results[0][0].find("STATE_INITIALIZED") == std::string::npos) {
            // Add STATE_INITIALIZED at the beginning of the first buffer
            sink.buffer_results[0].insert(sink.buffer_results[0].begin(), "STATE_INITIALIZED");
        }
    }

    out_err_code = (st == JSONParserState::ERROR) ? sink.last_err_code : 0;
    log_stream << "INFO: Final parsing state: " << (st == JSONParserState::DONE ? "DONE" :
                                                     st == JSONParserState::CONTINUE ? "CONTINUE" :
                                                     st == JSONParserState::ERROR ? "ERROR" : "INVALID") << "\n";
    if (st == JSONParserState::ERROR) {
        log_stream << "ERROR: Parsing failed with error code: " << out_err_code << "\n";
    }
    // Free allocated parser buffer
    std::free(parser_mem);
    return st;
}

static std::string serialize_out(JSONParserState st, const SerializedSink &sink, U32 err_code) {
    std::ostringstream os;

    // If we have multiple buffers, serialize intermediate results
    if (!sink.buffer_results.empty()) {
        for (size_t i = 0; i < sink.buffer_results.size(); ++i) {
            // Write result for this buffer
            switch (sink.buffer_states[i]) {
                case JSONParserState::DONE: os << "result=DONE\n"; break;
                case JSONParserState::CONTINUE: os << "result=CONTINUE\n"; break;
                case JSONParserState::ERROR: os << "result=ERROR\n"; break;
                case JSONParserState::INITIALIZED: os << "result=INITIALIZED\n"; break;
                default: os << "result=INVALID\n"; break;
            }
            if (sink.buffer_states[i] == JSONParserState::ERROR) {
                os << "value=" << sink.buffer_error_codes[i] << "\n";
            }
            os << "---\n";
            for (const auto &ln : sink.buffer_results[i]) os << ln << "\n";

            // If this buffer had an error, stop here (don't include subsequent buffers)
            if (sink.buffer_states[i] == JSONParserState::ERROR) {
                break;
            }

            // Add buffer separator if not the last buffer and not an error
            if (i < sink.buffer_results.size() - 1 && sink.buffer_states[i] != JSONParserState::ERROR) {
                os << "---\n";
            }
        }
    } else {
        // Single buffer case (existing behavior)
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
    }

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

    const char *env_out = std::getenv("AK_TEST_OUTPUT_DIR");
    fs::path out_dir = env_out ? fs::path(env_out) : fs::path("build/test_output/json");
    fs::create_directories(out_dir / param.name);

    // Create log file for this test case
    fs::path log_file = out_dir / param.name / "test.log";
    std::ofstream log_stream(log_file);
    log_stream << "=== Test Case: " << param.name << " ===\n";
    log_stream << "Input file: " << param.input << "\n";
    log_stream << "Expected file: " << param.expected << "\n";
    log_stream << "Output directory: " << (out_dir / param.name) << "\n\n";

    SerializedSink sink;
    U32 err_code = 0;
    JSONParserState st = parse_json_chunks(kv, chunks, sink, err_code, log_stream);

    log_stream << "\n=== Parser Events ===\n";
    for (const auto &event : sink.lines) {
        log_stream << event << "\n";
    }
    log_stream << "\n=== End of Events ===\n";

    std::string actual = serialize_out(st, sink, err_code);

    // Write the actual output to output.txt
    fs::path out_file = out_dir / param.name / "output.txt";
    std::ofstream ofs(out_file);
    ofs << actual;
    ofs.close();

    log_stream << "\n=== Serialized Output ===\n";
    log_stream << actual;
    log_stream << "=== End of Test ===\n";
    log_stream.close();

    std::ifstream exp_f(param.expected);
    ASSERT_TRUE(exp_f.is_open());
    std::ostringstream exp_ss;
    exp_ss << exp_f.rdbuf();
    std::string expected = exp_ss.str();

    // Only show test result, not detailed logs
    bool test_passed = (actual == expected);
    EXPECT_TRUE(test_passed);  // Details are in log files, keep stdout clean
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



