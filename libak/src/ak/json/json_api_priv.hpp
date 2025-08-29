#pragma once

#include "json_api.hpp"

namespace ak::json_priv {

// Mode constants
enum ParseMode {
    MODE_ROOT,
    MODE_VALUE,
    MODE_OBJECT,
    MODE_ARRAY,
    MODE_STRING,
    MODE_NUMBER,
    MODE_BOOLEAN,
    MODE_NULL,
    MODE_SKIP_WHITESPACE,
    // Sub-modes as needed, e.g., MODE_OBJECT_EXPECT_KEY, etc.
};

// Internal tail-recursive parse step
JSONParserState parse_step(const Char* buffer, Size remaining, Size pos, int mode, JSONParseContext* ctx);

// Other internal helpers if needed

} // namespace ak::json_priv
