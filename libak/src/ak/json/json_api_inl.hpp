#pragma once

#include "json_api.hpp"

namespace ak {


    inline U64 get_required_parse_session_buffer_size(JSONParseSessionConfig *cfg) noexcept 
    {
        U64 max_size = sizeof(JSONParseSession) + (cfg->max_depth * sizeof(JSONParseContext));
        return max_size + priv::CACHE_LINE_SIZE;
    }

} // namespace ak
