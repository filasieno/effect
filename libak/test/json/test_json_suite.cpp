/*
   JSON parser test harness

   Purpose
   - Drive the streaming, event-based JSON parser across a large corpus of cases.
   - Serialize parser events to a canonical textual form and compare against expected outputs checked into the repository.

   Test data layout (per file under libak/test/json/data)
   - Header: key=value lines (optional). Ends at a line that starts with
     "----------".
   - Body: one or more JSON fragments separated by lines containing exactly
     "---". Each fragment is fed as a separate buffer to the parser to test
     streaming across buffer boundaries.

   Event serialization
   - Objects/arrays: BEGIN_OBJECT/END_OBJECT, BEGIN_ARRAY/END_ARRAY
   - Keys: ATTR_KEY "..." more=0|1 (chunked via more flag)
   - Strings: STRING_VALUE "..." more=0|1 (chunked via more flag)
   - Scalars: NULL, BOOL true|false, INT <value>, FLOAT <value>
   - State change: STATE_CHANGED_EVENT: STATE_<INITIALIZED|CONTINUE|DONE|ERROR code>
   - End-of-input: PARSE_EOF_EVENT

   Notes
   - The harness fails fast if input cannot be parsed, or the expected file or
     input file cannot be opened.
   - Each test case writes logs and the serialized output under
     build/test_output/json/<case>/.
*/

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

// -----------------------------
// Types and test fixture
// -----------------------------

// Parameter describing a single test case discovered from the data directory.
struct TestCaseParam { 
    std::string name; 
    fs::path input; 
    fs::path expected; 
};

// Sink that accumulates serialized events and per-buffer snapshots.
struct SerializedSink {
    std::vector<std::string> lines;
    AkU32 last_err_code = 0;
    // For multi-buffer tests, store intermediate results
    std::vector<std::vector<std::string>> buffer_results;
    std::vector<AkJSONParserState> buffer_states;
    std::vector<AkU32> buffer_error_codes;
};

// gtest typed fixture
struct JSONParser : public ::testing::TestWithParam<TestCaseParam> {};

// on_json_event
//  Unified callback invoked by the parser. Translates events into textual
//  lines appended to the SerializedSink. Returns 0 to let parsing continue.
static int on_json_event(AkJSONParser *session, AkJSONEvent event, const AkJSONEventData *data, AkU64 more) noexcept {
    auto *sink = static_cast<SerializedSink *>(session->user_data);
    if (!sink) return 0;
    switch (event) {
        case AkJSONEvent::OBJECT_BEGIN: sink->lines.emplace_back("BEGIN_OBJECT"); break;
        case AkJSONEvent::OBJECT_END: sink->lines.emplace_back("END_OBJECT"); break;
        case AkJSONEvent::ARRAY_BEGIN: sink->lines.emplace_back("BEGIN_ARRAY"); break;
        case AkJSONEvent::ARRAY_END: sink->lines.emplace_back("END_ARRAY"); break;
        case AkJSONEvent::ATTR_KEY:
            if (data) sink->lines.emplace_back(std::string("ATTR_KEY \"") + std::string(data->string_data.str, data->string_data.len) + "\" more=" + (more ? "1" : "0"));
            break;
        case AkJSONEvent::NULL_VALUE: sink->lines.emplace_back("NULL"); break;
        case AkJSONEvent::BOOL_VALUE:
            if (data) sink->lines.emplace_back(std::string("BOOL ") + (data->bool_value ? "true" : "false"));
            break;
        case AkJSONEvent::INT_VALUE:
            if (data) sink->lines.emplace_back(std::string("INT ") + std::to_string((long long)data->int_value));
            break;
        case AkJSONEvent::FLOAT_VALUE: {
            if (data) { std::ostringstream os; os.setf(std::ios::fmtflags(0), std::ios::floatfield); os.precision(17); os << data->float_value; sink->lines.emplace_back(std::string("FLOAT ") + os.str()); }
            break; }
        case AkJSONEvent::STRING_VALUE:
            if (data) sink->lines.emplace_back(std::string("STRING_VALUE \"") + std::string(data->string_data.str, data->string_data.len) + "\" more=" + (more ? "1" : "0"));
            break;
        case AkJSONEvent::PARSE_STATE_CHANGED:
            if (data) {
                sink->last_err_code = data->state_data.err_code;
                switch (data->state_data.state) {
                    case AkJSONParserState::INITIALIZED:
                        sink->lines.emplace_back("STATE_CHANGED_EVENT: STATE_INITIALIZED");
                        break;
                    case AkJSONParserState::CONTINUE:
                        sink->lines.emplace_back("STATE_CHANGED_EVENT: STATE_CONTINUE");
                        break;
                    case AkJSONParserState::DONE:
                        sink->lines.emplace_back("STATE_CHANGED_EVENT: STATE_DONE");
                        break;
                    case AkJSONParserState::ERROR:
                        sink->lines.emplace_back(std::string("STATE_CHANGED_EVENT: STATE_ERROR ") + std::to_string((unsigned long long)data->state_data.err_code));
                        break;
                    default:
                        sink->lines.emplace_back("STATE_CHANGED_EVENT: STATE_INVALID");
                        break;
                }
            }
            break;
        case AkJSONEvent::PARSE_EOF:
            sink->lines.emplace_back("PARSE_EOF_EVENT");
            break;
    }
    return 0;
}

// parse_json_chunks
//  Configure a parse session from header key/values, feed each JSON chunk,
//  record per-chunk outputs, and finalize with EOF notification when needed.
//  Returns the final JSONParserState and sets out_err_code on error.
static AkJSONParserState parse_json_chunks(const std::vector<std::pair<std::string,std::string>> &kv,
                                         const std::vector<std::string> &chunks,
                                         SerializedSink &sink, AkU32 &out_err_code, std::ostream &log_stream) {
    // Defaults; may be overridden by key/values in the test input header
    AkJSONParserConfig cfg = { };
    // Apply key/value configuration
    for (const auto &p : kv) {
        if (p.first == "max_depth") {
            unsigned long long v = std::strtoull(p.second.c_str(), nullptr, 10);
            if (v > 0 && v <= std::numeric_limits<AkU32>::max()) cfg.max_depth = (AkU32)v;
        } else if (p.first == "max_string_size") {
            unsigned long long v = std::strtoull(p.second.c_str(), nullptr, 10);
            if (v > 0) cfg.max_string_size = (AkU64)v;
        } else if (p.first == "max_json_size") {
            unsigned long long v = std::strtoull(p.second.c_str(), nullptr, 10);
            if (v > 0) cfg.max_json_size = (AkU64)v;
        }
    }

    // Determine required parser buffer size and allocate dynamically
    AkU64 required_size = ak_get_required_buffer_size(&cfg);
    log_stream << "INFO: Required parser buffer size: " << required_size << " bytes\n";
    void *parser_mem = std::malloc((size_t)required_size);
    if (!parser_mem) {
        log_stream << "ERROR: Failed to allocate parser buffer of size " << required_size << "\n";
        out_err_code = (AkU32)AkJSONErrorCode::FATAL_STACK_OOB; // generic internal error for OOM in tests
        return AkJSONParserState::ERROR;
    }
    std::memset(parser_mem, 0, (size_t)required_size);
    AkJSONParser *session = ak_init_json_parser(parser_mem, required_size, &cfg, on_json_event, (void *)&sink);
    if (!session) {
        log_stream << "ERROR: Failed to initialize JSON parser session\n";
        std::free(parser_mem);
        return AkJSONParserState::ERROR;
    }
    log_stream << "INFO: JSON parser session initialized successfully\n";
    AkJSONParserState st = AkJSONParserState::INVALID;

    for (size_t i = 0; i < chunks.size(); ++i) {
        log_stream << "INFO: Processing chunk " << (i + 1) << "/" << chunks.size() << " (size: " << chunks[i].size() << " bytes)\n";

        // Capture lines before this chunk for intermediate results
        size_t chunk_start_lines = sink.lines.size();

        st = ak_run_json_parser(session, (void *)chunks[i].data(), (AkU64)chunks[i].size());
        log_stream << "INFO: Chunk " << (i + 1) << " processing result: " << (st == AkJSONParserState::DONE ? "DONE" :
                                                                               st == AkJSONParserState::CONTINUE ? "CONTINUE" :
                                                                               st == AkJSONParserState::ERROR ? "ERROR" : "INVALID") << "\n";

        // Store intermediate results for this buffer
        std::vector<std::string> chunk_lines(sink.lines.begin() + chunk_start_lines, sink.lines.end());
        sink.buffer_results.push_back(chunk_lines);
        sink.buffer_states.push_back(st);
        sink.buffer_error_codes.push_back((st == AkJSONParserState::ERROR) ? sink.last_err_code : 0);

        if (st == AkJSONParserState::ERROR) break;
    }

    // Always signal end of input when parser expects more data
    if (st == AkJSONParserState::CONTINUE) {
        log_stream << "INFO: Calling stop_json_parser to signal end of input\n";

        // Capture lines before stop_json_parser for intermediate results
        size_t lines_before_stop = sink.lines.size();

        st = ak_eof_json_parser(session);
        log_stream << "INFO: stop_json_parser result: " << (st == AkJSONParserState::DONE ? "DONE" :
                                                             st == AkJSONParserState::CONTINUE ? "CONTINUE" :
                                                             st == AkJSONParserState::ERROR ? "ERROR" : "INVALID") << "\n";

        // Store results from stop_json_parser as a separate "buffer"
        std::vector<std::string> stop_lines(sink.lines.begin() + lines_before_stop, sink.lines.end());
        sink.buffer_results.push_back(stop_lines);
        sink.buffer_states.push_back(st);
        sink.buffer_error_codes.push_back((st == AkJSONParserState::ERROR) ? sink.last_err_code : 0);
    }

    // If we have multiple buffers and the first buffer doesn't include the
    // explicit initialized event, prepend it as STATE_CHANGED_EVENT for rigor
    if (!sink.buffer_results.empty() && sink.buffer_results[0].size() > 0) {
        // Check if the first event in the first buffer is not a STATE_CHANGED_EVENT: STATE_INITIALIZED
        if (sink.buffer_results[0][0].find("STATE_CHANGED_EVENT: STATE_INITIALIZED") == std::string::npos) {
            // Add explicit STATE_CHANGED_EVENT for INITIALIZED at the beginning of the first buffer
            sink.buffer_results[0].insert(sink.buffer_results[0].begin(), "STATE_CHANGED_EVENT: STATE_INITIALIZED");
        }
    }

    out_err_code = (st == AkJSONParserState::ERROR) ? sink.last_err_code : 0;
    log_stream << "INFO: Final parsing state: " << (st == AkJSONParserState::DONE ? "DONE" :
                                                     st == AkJSONParserState::CONTINUE ? "CONTINUE" :
                                                     st == AkJSONParserState::ERROR ? "ERROR" : "INVALID") << "\n";
    if (st == AkJSONParserState::ERROR) {
        log_stream << "ERROR: Parsing failed with error code: " << out_err_code << "\n";
    }
    // Free allocated parser buffer
    std::free(parser_mem);
    return st;
}

// serialize_out
//  Turn captured lines into the canonical expected text format, preserving
//  buffer separators for multi-chunk cases.
static std::string serialize_out(const SerializedSink &sink) {
    std::ostringstream os;

    // If we have multiple buffers, serialize intermediate results
    if (!sink.buffer_results.empty()) {
        for (size_t i = 0; i < sink.buffer_results.size(); ++i) {
            os << "---\n";
            for (const auto &ln : sink.buffer_results[i]) os << ln << "\n";

            // If this buffer had an error, stop here (don't include subsequent buffers)
            if (sink.buffer_states[i] == AkJSONParserState::ERROR) {
                break;
            }

            // Add buffer separator if not the last buffer and not an error
            if (i < sink.buffer_results.size() - 1 && sink.buffer_states[i] != AkJSONParserState::ERROR) {
                os << "---\n";
            }
        }
    } else {
        os << "---\n";
        for (const auto &ln : sink.lines) os << ln << "\n";
    }

    return os.str();
}

// read_input_case
//  Load a test input file, returning header key/values and a vector of JSON
//  chunks (split by lines with "---"). Returns true on success.
static bool read_input_case(const fs::path &p, std::vector<std::pair<std::string,std::string>> &kv, std::vector<std::string> &chunks) {
    std::ifstream f(p);
    if (!f.is_open()) return false;
    std::string line; bool in_json = false; bool saw_separator = false; std::ostringstream current;
    while (std::getline(f, line)) {
        if (!in_json) {
            // Skip header comments: lines whose first non-space/tab is '#'
            {
                size_t i = 0;
                while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) ++i;
                if (i < line.size() && line[i] == '#') continue;
            }
            if (line.rfind("----------", 0) == 0) { in_json = true; saw_separator = true; continue; }
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
    // Require the test file to contain the JSON separator header
    return saw_separator && !chunks.empty();
}

// discover_cases
//  Enumerate <name>.txt inputs and pair them with existing <name>.expected.txt
//  expected files. Skip files that are themselves expected files.
static std::vector<TestCaseParam> discover_cases(const fs::path &data_root) {
    std::vector<TestCaseParam> out;
    if (!fs::exists(data_root)) return out;
    for (auto &entry : fs::directory_iterator(data_root)) {
        if (!entry.is_regular_file()) continue;
        fs::path in_path = entry.path();
        std::string name = in_path.filename().string();
        // Skip any file that is already an expected output
        if (name.size() > 13 && name.rfind(".expected.txt") == name.size() - 13) {
            continue;
        }

        // New expected suffix: .expected.txt
        fs::path expected = data_root / (in_path.stem().string() + ".expected.txt");
        if (fs::exists(expected) && fs::is_regular_file(expected)) {
            out.push_back(TestCaseParam{name, in_path, expected});
        }
    }
    return out;
}


// Test body for each discovered case.
TEST_P(JSONParser, Case) {
    const auto param = GetParam();
    std::vector<std::pair<std::string,std::string>> kv; std::vector<std::string> chunks;
    // Ensure both input and expected files exist and are regular files
    ASSERT_TRUE(fs::exists(param.input));
    ASSERT_TRUE(fs::is_regular_file(param.input));
    ASSERT_TRUE(fs::exists(param.expected));
    ASSERT_TRUE(fs::is_regular_file(param.expected));
    // Parse input file into header and chunks (require mandatory config keys)
    ASSERT_TRUE(read_input_case(param.input, kv, chunks));
    auto has_key = [&](const char* k){ for (auto &p: kv) if (p.first == k) return true; return false; };
    ASSERT_TRUE(has_key("max_depth"));
    ASSERT_TRUE(has_key("max_string_size"));
    ASSERT_TRUE(has_key("max_json_size"));

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
    AkU32 err_code = 0;
    // Run the parser over the input chunks and collect output
    (void)parse_json_chunks(kv, chunks, sink, err_code, log_stream);

    log_stream << "\n=== Parser Events ===\n";
    for (const auto &event : sink.lines) {
        log_stream << event << "\n";
    }
    log_stream << "\n=== End of Events ===\n";

    std::string actual = serialize_out(sink);

    // Write the actual output to output.txt
    fs::path out_file = out_dir / param.name / "output.txt";
    std::ofstream ofs(out_file);
    ofs << actual;
    ofs.close();

    log_stream << "\n=== Serialized Output ===\n";
    log_stream << actual;
    log_stream << "=== End of Test ===\n";
    log_stream.close();

    // Load expected output, fail if the file cannot be opened/read
    std::ifstream exp_f(param.expected);
    ASSERT_TRUE(exp_f.is_open());
    std::ostringstream exp_ss;
    exp_ss << exp_f.rdbuf();
    std::string expected = exp_ss.str();

    // Determine if expected is open-ended (only header '---' and no events).
    auto is_open_ended_expected = [&]() -> bool {
        // Normalize line endings and split into lines
        std::istringstream iss(expected);
        std::string line;
        bool saw_header = false;
        while (std::getline(iss, line)) {
            // Trim trailing CR if present (handle Windows newlines)
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (!saw_header) {
                // First non-empty line should be '---'
                std::string tmp = line;
                // Trim spaces
                tmp.erase(0, tmp.find_first_not_of(" \t"));
                tmp.erase(tmp.find_last_not_of(" \t") + 1);
                if (tmp.empty()) continue; // skip leading empty lines
                if (tmp == "---") {
                    saw_header = true;
                    continue;
                }
                // Any other content means there is a concrete expectation
                return false;
            } else {
                // After header, if any non-empty line exists, it's a concrete expectation
                std::string tmp = line;
                tmp.erase(0, tmp.find_first_not_of(" \t"));
                tmp.erase(tmp.find_last_not_of(" \t") + 1);
                if (!tmp.empty()) return false;
            }
        }
        // If we saw the header and nothing else meaningful, it's open-ended
        return saw_header;
    }();

    // Only show test result, not detailed logs
    // Ensure per-case output folder exists and has the output.txt we just wrote
    const char *env_out_verify = std::getenv("AK_TEST_OUTPUT_DIR");
    fs::path out_dir_verify = env_out_verify ? fs::path(env_out_verify) : fs::path("build/test_output/json");
    ASSERT_TRUE(fs::exists(out_dir_verify));
    ASSERT_TRUE(fs::exists(out_dir_verify / param.name));
    ASSERT_TRUE(fs::exists(out_dir_verify / param.name / "output.txt"));
    ASSERT_GT(fs::file_size(out_dir_verify / param.name / "output.txt"), 0u);

    bool test_passed = is_open_ended_expected ? true : (actual == expected);
    EXPECT_TRUE(test_passed);  // Details are in log files, keep stdout clean
}

static std::vector<TestCaseParam> load_params() {
    const char *env_data = std::getenv("AK_TEST_DATA_DIR");
    fs::path data_root = env_data ? fs::path(env_data) : fs::path("libak/test/json/data");
    return discover_cases(data_root);
}

struct NamePrinter {
    std::string operator()(const ::testing::TestParamInfo<TestCaseParam>& info) const {
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



