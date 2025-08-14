#pragma once

// -----------------------------------------------------------------------------



// -----------------------------------------------------------------------------

namespace ak { 
    
    using U64  = __u64;  
    using U32  = __u32; 
    using Size = __SIZE_TYPE__; 

    namespace priv {

        #ifdef NDEBUG
            constexpr bool IS_DEBUG_MODE    = false;
        #else
            constexpr bool IS_DEBUG_MODE    = true;   
        #endif
        
        constexpr bool TRACE_DEBUG_CODE = false;

    } // namespace ak::priv
} // namespace ak