#pragma once

namespace ak { 
    using Void  = void;
    using Bool  = bool;
    using Char  = char;
    using WChar = wchar_t;

    using U64  = unsigned long long;  
    using U32  = unsigned long; 
    using U16  = unsigned short;
    using U8   = unsigned char;

    using I64  = signed long long;  
    using I32  = signed int; 
    using I16  = signed short;
    using I8   = signed char;
    
    using Size = unsigned long long; 
    using ISize = signed long long; 
    using PtrDiff = signed long long;

    using F32 = float;
    using F64 = double;    

    namespace priv {

        #ifdef NDEBUG
            constexpr Bool IS_DEBUG_MODE = false;
        #else
            constexpr Bool IS_DEBUG_MODE = true;   
        #endif
        constexpr Bool ENABLE_AVX2      = false;
        constexpr Bool TRACE_DEBUG_CODE = false;

        constexpr U64 CACHE_LINE = 64;
    } // namespace ak::priv

} // namespace ak

#define AK_PACKED_ATTR __attribute__((packed))
#define AK_OFFSET(TYPE, MEMBER) ((::ak::Size)((U64)&(((TYPE*)0)->MEMBER)))