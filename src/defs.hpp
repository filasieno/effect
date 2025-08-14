#pragma once

// -----------------------------------------------------------------------------

/// \defgroup Task Task API
/// \brief Task API defines the API for creating and managing tasks.

/// \defgroup Kernel Kernel API
/// \brief Kernel API defines system level APIs.

// -----------------------------------------------------------------------------

namespace ak { 
    
    using U64  = __u64;  
    using U32  = __u32; 
    using Size = __SIZE_TYPE__; 

    namespace internal {

        #ifdef NDEBUG
            constexpr bool IS_DEBUG_MODE    = false;
        #else
            constexpr bool IS_DEBUG_MODE    = true;   
        #endif
        
        constexpr bool TRACE_DEBUG_CODE = false;

    } // namespace ak::internal
} // namespace ak