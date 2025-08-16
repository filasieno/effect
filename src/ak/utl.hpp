#pragma once

#include <cassert>
#include "ak/defs.hpp" // IWYU pragma: keep

namespace ak { namespace utl {

    struct DLink {
        DLink* next;
        DLink* prev;
    };

    inline void   InitLink(DLink* link) noexcept;
    inline bool   IsLinkDetached(const DLink* link) noexcept;
    inline void   DetachLink(DLink* link) noexcept;
    inline void   ClearLink(DLink* link) noexcept;
    inline void   EnqueueLink(DLink* queue, DLink* link);
    inline DLink* DequeueLink(DLink* queue);
    inline void   InsertPrevLink(DLink* queue, DLink* link);
    inline void   InsertNextLink(DLink* queue, DLink* link);
    inline void   PushLink(DLink* stack, DLink* link);
    inline DLink* PopLink(DLink* stack);

}} // namespace ak::utl


namespace ak { namespace priv {
    // alias DLink and functions in priv namespace
    using DLink = utl::DLink;
    using utl::InitLink;
    using utl::IsLinkDetached;
    using utl::DetachLink;
    using utl::ClearLink;
    using utl::EnqueueLink;
    using utl::DequeueLink;
    using utl::InsertPrevLink;
    using utl::InsertNextLink;
    using utl::PushLink;
    using utl::PopLink;
}} // namespace ak::priv
