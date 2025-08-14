#pragma once

#include "ak/utl.hpp"

namespace ak { namespace utl {

    inline void InitLink(DLink* link) noexcept {
        assert(link != nullptr);
        
        link->next = link;
        link->prev = link;
    }

    inline bool IsLinkDetached(const DLink* link) noexcept {
        assert(link != nullptr);
        assert(link->next != nullptr);
        assert(link->prev != nullptr);

        return link->next == link && link->prev == link;
    }

    inline void DetachLink(DLink* link) noexcept {
        assert(link != nullptr);
        assert(link->next != nullptr);
        assert(link->prev != nullptr);
        
        if (IsLinkDetached(link)) return;
        link->next->prev = link->prev;
        link->prev->next = link->next;
        link->next = link;
        link->prev = link;
    }

    inline void ClearLink(DLink* link) noexcept {
        assert(link != nullptr);
        
        link->next = nullptr;
        link->prev = nullptr;
    }

    inline void EnqueueLink(DLink* queue, DLink* link) {
        assert(queue != nullptr);
        assert(link != nullptr);
        assert(queue->next != nullptr);
        assert(queue->prev != nullptr);

        link->next = queue->next;
        link->prev = queue;

        link->next->prev = link;
        queue->next = link;
    }

    inline DLink* DequeueLink(DLink* queue) {
        assert(queue != nullptr);
        assert(queue->next != nullptr);
        assert(queue->prev != nullptr);
        if (IsLinkDetached(queue)) return nullptr;
        DLink* target = queue->prev;
        DetachLink(target);
        return target;
    }

    inline void InsertPrevLink(DLink* queue, DLink* link) {
        assert(queue != nullptr);
        assert(link != nullptr);
        assert(queue->next != nullptr);
        assert(queue->prev != nullptr);

        link->next = queue;
        link->prev = queue->prev;

        link->next->prev = link;
        link->prev->next = link;
    }

    inline void InsertNextLink(DLink* queue, DLink* link) {
        assert(queue != nullptr);
        assert(link != nullptr);
        assert(queue->next != nullptr);
        assert(queue->prev != nullptr);

        link->next = queue->next;
        link->prev = queue;

        link->next->prev = link;
        queue->next = link;
    }

    inline void PushLink(DLink* stack, DLink* link) {
        InsertNextLink(stack, link);
    }

    inline DLink* PopLink(DLink* stack) {
        assert(stack != nullptr);
        assert(stack->next != nullptr);
        assert(stack->prev != nullptr);
        assert(!IsLinkDetached(stack));
        
        DLink* target = stack->next;
        DetachLink(target);
        return target;
    }

}} // namespace ak::utl