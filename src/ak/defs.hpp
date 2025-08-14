#pragma once

namespace ak { 
    
    using U64  = unsigned long long;  
    using U32  = unsigned long; 
    using Size = unsigned long long; 

    namespace priv {

        #ifdef NDEBUG
            constexpr bool IS_DEBUG_MODE    = false;
        #else
            constexpr bool IS_DEBUG_MODE    = true;   
        #endif
        
        constexpr bool TRACE_DEBUG_CODE = false;

    } // namespace ak::priv
} // namespace ak