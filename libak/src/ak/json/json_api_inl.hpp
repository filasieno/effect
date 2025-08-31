#pragma once

#include "json_api.hpp"

namespace ak {


    inline AkU64 get_required_parse_session_buffer_size(JSONParseSessionConfig *cfg) noexcept 
    {
        AkU64 max_size = sizeof(JSONParseSession) + (cfg->max_depth * sizeof(JSONParseContext));
        return max_size + AK_CACHE_LINE_SIZE;
    }

} // namespace ak
