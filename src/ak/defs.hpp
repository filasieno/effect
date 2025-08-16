#pragma once

namespace ak { 
    using Void = void;
    using Bool = bool;
    using Char = char;
    using WChar = wchar_t;

    using U64  = unsigned long long;  
    using U32  = unsigned long; 
    using U16  = unsigned short;
    using U8   = unsigned char;

    using I64  = signed long long;  
    using I32  = signed long; 
    using I16  = signed short;
    using I8   = signed char;
    
    using Size = unsigned long long; 
    using ISize = signed long long; 
    using PtrDiff = signed long long;

    using F32 = float;
    using F64 = double;    

    namespace priv {

        #ifdef NDEBUG
            constexpr bool IS_DEBUG_MODE    = false;
        #else
            constexpr bool IS_DEBUG_MODE    = true;   
        #endif
        constexpr bool ENABLE_AVX2      = false;
        constexpr bool TRACE_DEBUG_CODE = false;

    } // namespace ak::priv
} // namespace ak