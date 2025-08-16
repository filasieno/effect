#pragma once

#include "ak/utl.hpp"

namespace ak { namespace utl {

    inline void init_link(DLink* link) noexcept {
        assert(link != nullptr);
        
        link->next = link;
        link->prev = link;
    }

    inline bool is_link_detached(const DLink* link) noexcept {
        assert(link != nullptr);
        assert(link->next != nullptr);
        assert(link->prev != nullptr);

        return link->next == link && link->prev == link;
    }

    inline void detach_link(DLink* link) noexcept {
        assert(link != nullptr);
        assert(link->next != nullptr);
        assert(link->prev != nullptr);
        
        if (is_link_detached(link)) return;
        link->next->prev = link->prev;
        link->prev->next = link->next;
        link->next = link;
        link->prev = link;
    }

    inline void clear_link(DLink* link) noexcept {
        assert(link != nullptr);
        
        link->next = nullptr;
        link->prev = nullptr;
    }

    inline void enqueue_link(DLink* queue, DLink* link) {
        assert(queue != nullptr);
        assert(link != nullptr);
        assert(queue->next != nullptr);
        assert(queue->prev != nullptr);

        link->next = queue->next;
        link->prev = queue;

        link->next->prev = link;
        queue->next = link;
    }

    inline DLink* dequeue_link(DLink* queue) {
        assert(queue != nullptr);
        assert(queue->next != nullptr);
        assert(queue->prev != nullptr);
        if (is_link_detached(queue)) return nullptr;
        DLink* target = queue->prev;
        detach_link(target);
        return target;
    }

    inline void insert_prev_link(DLink* queue, DLink* link) {
        assert(queue != nullptr);
        assert(link != nullptr);
        assert(queue->next != nullptr);
        assert(queue->prev != nullptr);

        link->next = queue;
        link->prev = queue->prev;

        link->next->prev = link;
        link->prev->next = link;
    }

    inline void insert_next_link(DLink* queue, DLink* link) {
        assert(queue != nullptr);
        assert(link != nullptr);
        assert(queue->next != nullptr);
        assert(queue->prev != nullptr);

        link->next = queue->next;
        link->prev = queue;

        link->next->prev = link;
        queue->next = link;
    }

    inline void push_link(DLink* stack, DLink* link) {
        insert_next_link(stack, link);
    }

    inline DLink* pop_link(DLink* stack) {
        assert(stack != nullptr);
        assert(stack->next != nullptr);
        assert(stack->prev != nullptr);
        assert(!is_link_detached(stack));
        
        DLink* target = stack->next;
        detach_link(target);
        return target;
    }

}} // namespace ak::utl