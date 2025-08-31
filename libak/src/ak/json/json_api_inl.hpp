#pragma once

#include "json_api.hpp"

inline AkU64 ak_get_required_buffer_size(AkJSONParserConfig *cfg) noexcept 
{
    AkU64 max_size = sizeof(AkJSONParser) + (cfg->max_depth * sizeof(AkJSONParserCtx));
    return max_size + AK_CACHE_LINE_SIZE;
}
