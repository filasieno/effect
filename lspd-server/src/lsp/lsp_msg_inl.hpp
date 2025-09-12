#pragma once

#include "lsp_msg.hpp" // IWYU pragma: keep

// static inline void lsp_msg_retain(lsp_msg_ref ref) noexcept {
//     if (ref == nullptr) {
//         return;
//     }
//     lsp_msg_hdr *hdr = static_cast<lsp_msg_hdr *>(ref);
//     hdr->refcount.fetch_add(1, std::memory_order_relaxed);
// }

// static inline AkU32 lsp_msg_release(lsp_msg_ref ref) noexcept {
//     if (ref == nullptr) {
//         return 0u;
//     }
//     lsp_msg_hdr *hdr = static_cast<lsp_msg_hdr *>(ref);
//     AkU32 prev = hdr->refcount.fetch_sub(1, std::memory_order_acq_rel);
//     // When prev == 1 the refcount reached zero. Actual reclamation is delegated to the GC store.
//     return (prev > 0u) ? (prev - 1u) : 0u;
// }

// /// \ingroup lsp_base_types
// /// \brief Initialize message header with default values
// /// 
// /// Sets up a message header with default values suitable for
// /// initialization. Timestamps the message and sets up reference counting.
// /// 
// /// \param hdr Pointer to message header to initialize
// /// 
// static inline void lsp_msg_hdr_init(lsp_msg_hdr *hdr) noexcept {
//     hdr->timestamp_nanos = ak_query_timer_ns();
//     hdr->refcount.store(1u, std::memory_order_relaxed);
//     hdr->op_tag = 0u;
//     hdr->tag_id = 0u;
// }

// /// \ingroup lsp_base_types
// /// \brief Create operation tag from method and interaction
// /// 
// /// Combines method identifier and interaction kind into a single
// /// operation tag for efficient message routing and correlation.
// /// 
// /// \param method LSP method identifier
// /// \param interaction Message interaction kind
// /// \return Combined operation tag
// /// 
// static inline AkU64 lsp_make_op_tag(lsp_method_type method, lsp_interaction_kind interaction) noexcept {
//     return ((AkU64)((AkU64)interaction & 0xFFFFull) << 32) | (AkU64)((AkU64)method & 0xFFFFFFFFull);
// }

// /// \ingroup lsp_base_types
// /// \brief Initialize message header with method and interaction
// /// 
// /// Sets up a complete message header with method, interaction, and
// /// correlation information. This is the primary initialization function
// /// for new messages.
// /// 
// /// \param hdr Pointer to message header to initialize
// /// \param interaction Message interaction kind
// /// \param method LSP method identifier
// /// \param tag_id Unique correlation tag
// /// 
// static inline void lsp_msg_hdr_init_with_op(lsp_msg_hdr *hdr, lsp_interaction_kind interaction, lsp_method_type method, AkU64 tag_id) noexcept {
//     lsp_msg_hdr_init(hdr);
//     hdr->interaction = interaction;
//     hdr->method = method;
//     hdr->op_tag = lsp_make_op_tag(method, interaction);
//     hdr->tag_id = tag_id;
// }
