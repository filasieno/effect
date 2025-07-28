#pragma once

#ifdef NDEBUG
    constexpr bool DEFINED_DEBUG = false;
#else
    constexpr bool DEFINED_DEBUG = true;
#endif