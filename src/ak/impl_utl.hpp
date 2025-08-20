#pragma once

#include "ak/utl.hpp"

namespace ak { namespace utl {

    inline Void init_dlink(DLink* link) noexcept {
        assert(link != nullptr);
        
        link->next = link;
        link->prev = link;
    }

    inline Bool is_dlink_detached(const DLink* link) noexcept {
        assert(link != nullptr);
        assert(link->next != nullptr);
        assert(link->prev != nullptr);

        return link->next == link && link->prev == link;
    }

    inline Void detach_dlink(DLink* link) noexcept {
        assert(link != nullptr);
        assert(link->next != nullptr);
        assert(link->prev != nullptr);
        
        if (is_dlink_detached(link)) return;
        link->next->prev = link->prev;
        link->prev->next = link->next;
        link->next = link;
        link->prev = link;
    }

    inline Void clear_dlink(DLink* link) noexcept {
        assert(link != nullptr);
        
        link->next = nullptr;
        link->prev = nullptr;
    }

    inline Void enqueue_dlink(DLink* queue, DLink* link) noexcept {
        assert(queue != nullptr);
        assert(link != nullptr);
        assert(queue->next != nullptr);
        assert(queue->prev != nullptr);

        link->next = queue->next;
        link->prev = queue;

        link->next->prev = link;
        queue->next = link;
    }

    inline DLink* dequeue_dlink(DLink* queue) noexcept {
        assert(queue != nullptr);
        assert(queue->next != nullptr);
        assert(queue->prev != nullptr);
        if (is_dlink_detached(queue)) return nullptr;
        DLink* target = queue->prev;
        detach_dlink(target);
        return target;
    }

    inline Void insert_prev_dlink(DLink* queue, DLink* link) noexcept {
        assert(queue != nullptr);
        assert(link != nullptr);
        assert(queue->next != nullptr);
        assert(queue->prev != nullptr);

        link->next = queue;
        link->prev = queue->prev;

        link->next->prev = link;
        link->prev->next = link;
    }

    inline Void insert_next_dlink(DLink* queue, DLink* link) noexcept {
        assert(queue != nullptr);
        assert(link != nullptr);
        assert(queue->next != nullptr);
        assert(queue->prev != nullptr);

        link->next = queue->next;
        link->prev = queue;

        link->next->prev = link;
        queue->next = link;
    }

    inline Void push_dlink(DLink* stack, DLink* link) noexcept {
        insert_next_dlink(stack, link);
    }

    inline DLink* pop_dlink(DLink* stack) noexcept {
        assert(stack != nullptr);
        assert(stack->next != nullptr);
        assert(stack->prev != nullptr);
        assert(!is_dlink_detached(stack));
        
        DLink* target = stack->next;
        detach_dlink(target);
        return target;
    }

}} // namespace ak::utl