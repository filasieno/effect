#pragma once

#include "ak/api_priv.hpp" // IWYU pragma: keep

namespace ak { 


    namespace priv { 


        // AVL utility forward declarations 
        inline static Void                  clear(AllocFreeBlockHeader* link) noexcept;
        inline static Bool                  is_detached(const AllocFreeBlockHeader* link) noexcept;
        inline static I32                   height_of(const AllocFreeBlockHeader* n) noexcept;
        inline static Void                  update(AllocFreeBlockHeader* n) noexcept;
        inline static Void                  rotate_left(AllocFreeBlockHeader** root, AllocFreeBlockHeader* x) noexcept;
        inline static Void                  rotate_right(AllocFreeBlockHeader** root, AllocFreeBlockHeader* y) noexcept;
        inline static Void                  rebalance_upwards(AllocFreeBlockHeader** root, AllocFreeBlockHeader* n) noexcept;
        inline static Void                  transplant(AllocFreeBlockHeader** root, AllocFreeBlockHeader* u, AllocFreeBlockHeader* v) noexcept;
        inline static AllocFreeBlockHeader* min_node(AllocFreeBlockHeader* root) noexcept;

} }


namespace ak { namespace priv { 

    // (AVL utility implementations are located at the end of the file)

    inline Void init_free_block_tree_root(AllocFreeBlockHeader** root) noexcept {
        assert(root != nullptr);
        *root = nullptr;
    }

    inline AllocFreeBlockHeader* put_free_block(AllocFreeBlockHeader** root, AllocBlockHeader* block) noexcept {
        assert(root != nullptr);
        assert(block != nullptr);
        assert(block->this_desc.state == (U32)AllocBlockState::FREE);
        assert(block->this_desc.size >= 64);

        auto key_of = [](const AllocFreeBlockHeader* n) noexcept -> U64 { return n->this_desc.size; };
        // (helpers moved to static inline utilities above)

        AllocFreeBlockHeader* new_link = (AllocFreeBlockHeader*)block;

        if (*root == nullptr) {
            // First node becomes root (as tree node)
            new_link->height = 1;
            new_link->balance = 0;
            new_link->parent = nullptr;
            new_link->left = nullptr;
            new_link->right = nullptr;
            new_link->next = new_link;
            new_link->prev = new_link;
            *root = new_link;
            return new_link;
        }

        // Traverse to find insertion point or existing key
        AllocFreeBlockHeader* cur = *root;
        AllocFreeBlockHeader* parent = nullptr;
        U64 k = new_link->this_desc.size;
        while (cur) {
            parent = cur;
            U64 ck = key_of(cur);
            if (k == ck) {
                // Insert as list node at tail (FIFO semantics)
                new_link->height = -1; // mark as list node
                new_link->balance = 0;
                new_link->parent = nullptr;
                new_link->left = nullptr;
                new_link->right = nullptr;
                // append before head
                AllocFreeBlockHeader* head = cur;
                AllocFreeBlockHeader* tail = head->prev;
                new_link->next = head;
                new_link->prev = tail;
                tail->next = new_link;
                head->prev = new_link;
                return new_link;
            } else if (k < ck) {
                cur = cur->left;
            } else {
                cur = cur->right;
            }
        }

        // Insert as AVL node under parent
        new_link->height = 1;
        new_link->balance = 0;
        new_link->left = nullptr;
        new_link->right = nullptr;
        new_link->next = new_link;
        new_link->prev = new_link;
        new_link->parent = parent;
        if (k < key_of(parent)) parent->left = new_link; else parent->right = new_link;

        // Rebalance up to root
        rebalance_upwards(root, parent);
        return new_link;
    }

    inline AllocFreeBlockHeader* find_gte_free_block(AllocFreeBlockHeader** root, U64 block_size) noexcept {
        assert(root != nullptr);
        assert(block_size >= 64);
        if (*root == nullptr) return nullptr;

        AllocFreeBlockHeader* node = *root;
        AllocFreeBlockHeader* best = nullptr;
        while (node) {
            U64 k = node->this_desc.size;
            if (k == block_size) return node;
            if (k > block_size) { best = node; node = node->left; }
            else { node = node->right; }
        }
        return best;
    }
    
    inline AllocBlockHeader* detach_free_block(AllocFreeBlockHeader** root, AllocFreeBlockHeader* link) noexcept {
        assert(root != nullptr);
        assert(*root != nullptr);
        assert(link != nullptr);
        assert(link->this_desc.state == (U32)AllocBlockState::FREE);
        
        // Case 1: List node case; the node is part of a list; just unlink it
        // It is guarateed that root is stable 
        // Nothing ever to rebalance
        if (link->height < 0) {
            AllocFreeBlockHeader* prev = link->prev;
            AllocFreeBlockHeader* next = link->next;
            prev->next = next;
            next->prev = prev;
            clear(link);
           
            return link;
        }
        assert(link->height >= 0);
        
        // Helpers for AVL maintenance
        // (helpers moved to static inline utilities above)

        // Case 2: Simple AVL tree node case; there is no list node linked in the tree
        if (is_detached(link)) {
            AllocFreeBlockHeader* start_rebalance = link->parent;
            if (link->left == nullptr) {
                transplant(root, link, link->right);
            } else if (link->right == nullptr) {
                transplant(root, link, link->left);
            } else {
                AllocFreeBlockHeader* s = min_node(link->right);
                if (s->parent != link) {
                    // Replace s with its right subtree
                    AllocFreeBlockHeader* sp = s->parent;
                    transplant(root, s, s->right);
                    // Attach original right to s
                    s->right = link->right;
                    if (s->right) s->right->parent = s;
                    start_rebalance = sp;
                } else {
                    start_rebalance = s;
                }
                // Replace link with s
                transplant(root, link, s);
                s->left = link->left;
                if (s->left) s->left->parent = s;
                update(s);
            }
            // Clear the detached node and rebalance
            clear(link);
            if (*root) rebalance_upwards(root, start_rebalance);
            return link;
        }

        // Case 3: Tree node case; the node is part of a tree and it is also the head of a list.
        // 
        // We have to execute swap the tree node with the first node in the link (FIFO)
        //
        // 1. Get the first element of the list N (FIFO) and detach H from the ring
        AllocFreeBlockHeader* H = link;
        AllocFreeBlockHeader* N = H->next;
        assert(N != nullptr && N != H);
        // Remove H from circular list so that N becomes the new head
        H->prev->next = H->next;
        H->next->prev = H->prev;
        // H becomes a detached single-node ring
        H->next = H;
        H->prev = H;

        // 3. Replace in the tree the node H with the node N
        N->height = H->height;
        N->balance = H->balance;
        N->left = H->left;
        N->right = H->right;
        N->parent = H->parent;
        if (N->left) N->left->parent = N;
        if (N->right) N->right->parent = N;
        if (H->parent == nullptr) {
            *root = N;
        } else if (H->parent->left == H) {
            H->parent->left = N;
        } else {
            H->parent->right = N;
        }

        // 4. Clear H and return it
        clear(H);
        return H;

    }

    // ------------------------------------------------------------------
    // AVL utility implementations (moved to bottom for clarity)

    inline static Void clear(AllocFreeBlockHeader* link) noexcept {
        assert(link != nullptr);
        char* buff = ((char*)link) + sizeof(AllocBlockHeader);
        std::memset(buff, 0, sizeof(AllocFreeBlockHeader) - sizeof(AllocBlockHeader));
    }

    inline static Bool is_detached(const AllocFreeBlockHeader* link) noexcept {
        assert(link != nullptr);
        return link->next == link && link->prev == link;
    }

    inline static I32 height_of(const AllocFreeBlockHeader* n) noexcept { return n ? n->height : 0; }

    inline static Void update(AllocFreeBlockHeader* n) noexcept {
        if (!n) return;
        const I32 hl = height_of(n->left);
        const I32 hr = height_of(n->right);
        n->height  = 1 + (hl > hr ? hl : hr);
        n->balance = hl - hr;
    }

    inline static Void rotate_left(AllocFreeBlockHeader** r, AllocFreeBlockHeader* x) noexcept {
        AllocFreeBlockHeader* y = x->right;
        assert(y != nullptr);
        x->right = y->left;
        if (y->left) y->left->parent = x;
        y->parent = x->parent;
        if (x->parent == nullptr) {
            *r = y;
        } else if (x->parent->left == x) {
            x->parent->left = y;
        } else {
            x->parent->right = y;
        }
        y->left = x;
        x->parent = y;
        update(x);
        update(y);
    }

    inline static Void rotate_right(AllocFreeBlockHeader** r, AllocFreeBlockHeader* y) noexcept {
        AllocFreeBlockHeader* x = y->left;
        assert(x != nullptr);
        y->left = x->right;
        if (x->right) x->right->parent = y;
        x->parent = y->parent;
        if (y->parent == nullptr) {
            *r = x;
        } else if (y->parent->left == y) {
            y->parent->left = x;
        } else {
            y->parent->right = x;
        }
        x->right = y;
        y->parent = x;
        update(y);
        update(x);
    }

    inline static Void rebalance_upwards(AllocFreeBlockHeader** r, AllocFreeBlockHeader* n) noexcept {
        while (n) {
            update(n);
            if (n->balance > 1) {
                if (n->left && n->left->balance < 0) {
                    rotate_left(r, n->left);
                }
                rotate_right(r, n);
            } else if (n->balance < -1) {
                if (n->right && n->right->balance > 0) {
                    rotate_right(r, n->right);
                }
                rotate_left(r, n);
            }
            n = n->parent;
        }
    }

    inline static Void transplant(AllocFreeBlockHeader** r, AllocFreeBlockHeader* u, AllocFreeBlockHeader* v) noexcept {
        if (u->parent == nullptr) {
            *r = v;
        } else if (u->parent->left == u) {
            u->parent->left = v;
        } else {
            u->parent->right = v;
        }
        if (v) v->parent = u->parent;
    }

    inline static AllocFreeBlockHeader* min_node(AllocFreeBlockHeader* n) noexcept {
        assert(n != nullptr);
        while (n->left) n = n->left;
        return n;
    }

}}
