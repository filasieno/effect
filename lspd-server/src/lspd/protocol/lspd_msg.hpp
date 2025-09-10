#pragma once

#include "lspd_basic.hpp"

/// \ingroup lsp_base_types
/// \brief Opaque reference to a message in the GC-managed store
/// 
/// Type-safe opaque pointer to messages stored in the garbage-collected
/// message store. All message pointers must point to structures that
/// begin with lsp_msg_hdr.
/// 
using lsp_msg_ref = void *;

/// \ingroup lsp_base_types
/// \brief Increment message reference count
/// 
/// Increases the reference count of a message to prevent premature
/// garbage collection. Must be called when storing additional references
/// to a message.
/// 
/// \param ref Message reference to retain (can be null)
/// 
static inline void lsp_msg_retain(lsp_msg_ref ref) noexcept;

/// \ingroup lsp_base_types
/// \brief Decrement message reference count
/// 
/// Decreases the reference count of a message. When the count reaches
/// zero, the message becomes eligible for garbage collection.
/// 
/// \param ref Message reference to release (can be null)
/// \return Previous reference count
/// 
static inline AkU32 lsp_msg_release(lsp_msg_ref ref) noexcept;

/// \ingroup lsp_base_types
/// \brief Initialize message header with default values
/// 
/// Sets up a message header with default values suitable for
/// initialization. Timestamps the message and sets up reference counting.
/// 
/// \param hdr Pointer to message header to initialize
/// 
static inline void lsp_msg_hdr_init(lsp_msg_hdr *hdr) noexcept;

/// \ingroup lsp_base_types
/// \brief Create operation tag from method and interaction
/// 
/// Combines method identifier and interaction kind into a single
/// operation tag for efficient message routing and correlation.
/// 
/// \param method LSP method identifier
/// \param interaction Message interaction kind
/// \return Combined operation tag
/// 
static inline AkU64 lsp_make_op_tag(enum lsp_method_type method, enum lsp_interaction_kind interaction) noexcept;
/// \ingroup lsp_base_types
/// \brief Initialize message header with method and interaction
/// 
/// Sets up a complete message header with method, interaction, and
/// correlation information. This is the primary initialization function
/// for new messages.
/// 
/// \param hdr Pointer to message header to initialize
/// \param interaction Message interaction kind
/// \param method LSP method identifier
/// \param tag_id Unique correlation tag
/// 
static inline void lsp_msg_hdr_init_with_op(struct lsp_msg_hdr *hdr, enum lsp_interaction_kind interaction, enum lsp_method_type method, AkU64 tag_id) noexcept;
