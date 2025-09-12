#pragma once

#include "ak/base/base_api.hpp"

/// \file dap_basic.hpp
/// \brief Base/common DAP (Debug Adapter Protocol) type declarations
///
/// This header defines base types used across the DAP implementation.
/// DAP types are kept independent from LSP types. Do NOT share types between modules.

// ======================================================================================================================
// BASIC TYPES (no cross-module dependencies)
// ======================================================================================================================

/// \ingroup dap_base_types
/// \brief String type for DAP
struct dap_string {
    const char* chars;
    int         length;
};

/// \ingroup dap_optional_types
/// \brief Optional string value (tagged union)
struct dap_opt_string {
    int         kind;   ///< 0 => none, 1 => some
    dap_string  value;  ///< Valid if kind == 1
};

/// \ingroup dap_optional_types
/// \brief Optional unsigned integer value
struct dap_opt_uinteger {
    int          kind;   ///< 0 => none, 1 => some
    unsigned int value;  ///< Valid if kind == 1
};

/// \ingroup dap_base_types
/// \brief Generic singly-linked list
template <typename T>
struct dap_list {
    T                item;
    struct dap_list* next;
};

/// \ingroup dap_base_types
/// \brief Message header for DAP protocol messages (internal housekeeping)
struct dap_msg_hdr {
    AkU64              timestamp_nanos; ///< Creation timestamp in nanoseconds
    std::atomic<AkU32> refcount;        ///< Atomic reference count for memory management
    AkU64              tag_id;          ///< Correlation tag for pipelines
};
static void dap_msg_hdr_init(struct dap_msg_hdr* hdr, AkU64 tag_id) noexcept;
// ======================================================================================================================
// DYNAMIC VALUE (independent from LSP)
// ======================================================================================================================

/// \ingroup dap_dynamic_types
/// \brief Discriminated union kind for dap_dyn
enum dap_dyn_kind {
    DAP_DYN_INVALID = 0,
    DAP_DYN_BOOLEAN_TRUE,
    DAP_DYN_BOOLEAN_FALSE,
    DAP_DYN_INTEGER,
    DAP_DYN_UINTEGER,
    DAP_DYN_DECIMAL,
    DAP_DYN_STRING,
    DAP_DYN_ARRAY,
    DAP_DYN_OBJECT
};

/// \ingroup dap_dynamic_types
/// \brief Minimal dynamic value shell used for arguments/body fields
struct dap_dyn {
    enum dap_dyn_kind kind;
};

// Forward declarations for composite dynamic subtypes (defined in dap_dyn.hpp if needed)
struct dap_dyn_bool;
struct dap_dyn_int;
struct dap_dyn_uint;
struct dap_dyn_decimal;
struct dap_dyn_string;
struct dap_dyn_array;
struct dap_dyn_object;

// ======================================================================================================================
// PROTOCOL BASE WRAPPERS
// ======================================================================================================================

/// \ingroup dap_protocol
/// \brief Base protocol message common fields (ProtocolMessage)
struct dap_protocol_message_base {
    struct dap_msg_hdr hdr;
    AkU32              seq;   ///< Sequence number (>=1 per sender)
};

/// \ingroup dap_protocol
/// \brief DAP Request base
struct dap_request_base : public dap_protocol_message_base
{    
    struct dap_string command;   ///< Request command
};

/// \ingroup dap_protocol
/// \brief DAP Response base
struct dap_response_base : public dap_protocol_message_base
{
    AkU32                           request_seq; ///< Sequence of corresponding request
    bool                            success;     ///< Outcome
    struct dap_string               command;     ///< Request command
    struct dap_opt_string           message;     ///< Error message if failed
};

/// \ingroup dap_protocol
/// \brief DAP Event base
struct dap_event_base : public dap_protocol_message_base
{
    struct dap_string event;   ///< Event name
};


