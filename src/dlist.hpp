#pragma once
#include <cassert>

struct DList {

    void init() {
        this->next = this;
        this->prev = this;
    }

    void detach() {
        if (detached()) return;
        this->next->prev = this->prev;
        this->prev->next = this->next;
        this->next = this;
        this->prev = this;
    }
    
    void push_front(DList* node) {
        assert(node != nullptr);
        assert(node->detached());
    
        node->prev = this->prev;
        node->next = this;
        
        node->prev->next = node;
        this->prev = node; 
    }

    void push_back(DList* node) {
        assert(node != nullptr);
        assert(node->detached());

        node->next = this->next;
        node->prev = this;
        
        node->next->prev = node;
        this->next = node;
    }

    DList* pop_front() {
        if (detached()) return nullptr;
        DList* target = this->prev;
        target->detach();
        return target;
    }

    DList* pop_back() {
        if (detached()) return nullptr;
        DList* target = this->next;
        target->detach();
        return target;
    }

    DList& front() {
        return *this->prev;
    }

    DList& back() {
        return *this->next;        
    }
    
    bool detached() const {
        return this->next == this && this->prev == this;
    }

    DList* next;
    DList* prev;
};


