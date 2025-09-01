#include "ak/alloc/alloc.hpp" // IWYU pragma: keep
#include <cstring>


// AVL utility forward declarations 

inline static AkI32                   alloc_freeblock_height_of(const AkAllocFreeBlockHeader* n) noexcept;
inline static AkVoid                  alloc_freeblock_update(AkAllocFreeBlockHeader* n) noexcept;
inline static AkVoid                  alloc_freeblock_rotate_left(AkAllocFreeBlockHeader** root, AkAllocFreeBlockHeader* x) noexcept;
inline static AkVoid                  alloc_freeblock_rotate_right(AkAllocFreeBlockHeader** root, AkAllocFreeBlockHeader* y) noexcept;
inline static AkVoid                  alloc_freeblock_rebalance_upwards(AkAllocFreeBlockHeader** root, AkAllocFreeBlockHeader* n) noexcept;
inline static AkVoid                  alloc_freeblock_transplant(AkAllocFreeBlockHeader** root, AkAllocFreeBlockHeader* u, AkAllocFreeBlockHeader* v) noexcept;
inline static AkAllocFreeBlockHeader* alloc_freeblock_min_node(AkAllocFreeBlockHeader* root) noexcept;

AkVoid alloc_freeblock_init_root(AkAllocFreeBlockHeader** root) noexcept {
    AK_ASSERT(root != nullptr);
    *root = nullptr;
}

AkVoid alloc_freeblock_put(AkAllocFreeBlockHeader** root, AkAllocBlockHeader* block) noexcept {
    AK_ASSERT(root != nullptr);
    AK_ASSERT(block != nullptr);
    AK_ASSERT(block->this_desc.state == (AkU32)AkAllocBlockState::FREE);
    AK_ASSERT(block->this_desc.size > 2048);

    auto key_of = [](const AkAllocFreeBlockHeader* n) noexcept -> AkU64 { return n->this_desc.size; };
    // (helpers moved to static inline utilities above)

    AkAllocFreeBlockHeader* new_link = (AkAllocFreeBlockHeader*)block;

    if (*root == nullptr) {
        // First node becomes root (as tree node)
        new_link->height = 1;
        new_link->balance = 0;
        new_link->parent = nullptr;
        new_link->left = nullptr;
        new_link->right = nullptr;
        ak_dlink_init(&new_link->multimap_link);
        *root = new_link;
        return;
    }

    // Traverse to find insertion point or existing key
    AkAllocFreeBlockHeader* cur = *root;
    AkAllocFreeBlockHeader* parent = nullptr;
    AkU64 k = new_link->this_desc.size;
    while (cur) {
        parent = cur;
        AkU64 ck = key_of(cur);
        if (k == ck) {
            // Insert as list node at tail (FIFO semantics)
            new_link->height = -1; // mark as list node
            new_link->balance = 0;
            new_link->parent = nullptr;
            new_link->left = nullptr;
            new_link->right = nullptr;
            // append before head (FIFO): head->next remains first inserted
            ak_dlink_insert_prev(&cur->multimap_link, &new_link->multimap_link);
            return;
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
    ak_dlink_init(&new_link->multimap_link);
    new_link->parent = parent;
    if (k < key_of(parent)) parent->left = new_link; else parent->right = new_link;

    // Rebalance up to root
    alloc_freeblock_rebalance_upwards(root, parent);
    return;
}

AkAllocFreeBlockHeader* alloc_freeblock_find_gte(AkAllocFreeBlockHeader* root, AkU64 block_size) noexcept {
    if (root == nullptr) return nullptr;
    if (block_size <= 2048) return nullptr;
    
    AkAllocFreeBlockHeader* node = root;
    AkAllocFreeBlockHeader* best = nullptr;
    while (node) {
        AkU64 k = node->this_desc.size;
        if (k == block_size) return node;
        if (k > block_size) { best = node; node = node->left; }
        else { node = node->right; }
    }
    return best;
}

AkVoid alloc_freeblock_detach(AkAllocFreeBlockHeader** root, AkAllocFreeBlockHeader* node) noexcept {
    AK_ASSERT(root != nullptr);
    AK_ASSERT(*root != nullptr);
    AK_ASSERT(node != nullptr);
    AK_ASSERT(node->this_desc.state == (AkU32)AkAllocBlockState::FREE);
    AK_ASSERT(node->this_desc.size > 2048);
    
    // Case 1: List node case; the node is part of a list; just unlink it
    // It is guarateed that root is stable 
    // Nothing ever to rebalance
    if (node->height < 0) {
        ak_dlink_detach(&node->multimap_link);
        alloc_freeblock_clear(node);
        return;
    }

    // Case 2: Simple AVL tree node case; there is no list node linked in the tree
    if (alloc_freeblock_is_detached(node)) {
        AkAllocFreeBlockHeader* start_rebalance = node->parent;
        if (node->left == nullptr) {
            alloc_freeblock_transplant(root, node, node->right);
        } else if (node->right == nullptr) {
            alloc_freeblock_transplant(root, node, node->left);
        } else {
            AkAllocFreeBlockHeader* s = alloc_freeblock_min_node(node->right);
            if (s->parent != node) {
                // Replace s with its right subtree
                AkAllocFreeBlockHeader* sp = s->parent;
                alloc_freeblock_transplant(root, s, s->right);
                // Attach original right to s
                s->right = node->right;
                if (s->right) s->right->parent = s;
                start_rebalance = sp;
            } else {
                start_rebalance = s;
            }
            // Replace link with s
            alloc_freeblock_transplant(root, node, s);
            s->left = node->left;
            if (s->left) s->left->parent = s;
            alloc_freeblock_update(s);
        }
        // Clear the detached node and rebalance
        alloc_freeblock_clear(node);
        if (*root) alloc_freeblock_rebalance_upwards(root, start_rebalance);
        return;
    }

    // Case 3: Tree node case; the node is part of a tree and it is also the head of a list.
    // 
    // We have to execute swap the tree node with the first node in the link (FIFO)
    //
    // 1. Get the first element of the list N (FIFO) and detach H from the ring
    
    AkDLink* next_node_link = node->multimap_link.next;
    AkAllocFreeBlockHeader* next_node = (AkAllocFreeBlockHeader*)((AkChar*)next_node_link - AK_OFFSET(AkAllocFreeBlockHeader, multimap_link));
    AK_ASSERT(next_node != nullptr && next_node != node);
    // Remove H from circular list so that N becomes the new head
    ak_dlink_detach(&node->multimap_link);
    // H becomes a detached single-node ring (already true after detach)

    // 2. Replace in the tree the node H with the node N
    next_node->height = node->height;
    next_node->balance = node->balance;
    next_node->left = node->left;
    next_node->right = node->right;
    next_node->parent = node->parent;
    if (next_node->left) next_node->left->parent = next_node;
    if (next_node->right) next_node->right->parent = next_node;
    if (node->parent == nullptr) {
        *root = next_node;
    } else if (node->parent->left == node) {
        node->parent->left = next_node;
    } else {
        node->parent->right = next_node;
    }

    // 4. Clear H and return it
    alloc_freeblock_clear(node);
    return;

}

AkBool alloc_freeblock_is_detached(const AkAllocFreeBlockHeader* link) noexcept {
    AK_ASSERT(link != nullptr);
    return link->multimap_link.next == &link->multimap_link && link->multimap_link.prev == &link->multimap_link;
}

// ------------------------------------------------------------------
// AVL utility implementations (moved to bottom for clarity)

AkVoid alloc_freeblock_clear(AkAllocFreeBlockHeader* link) noexcept {
    AK_ASSERT(link != nullptr);
    char* buff = ((char*)link) + sizeof(AkAllocBlockHeader);
    std::memset(buff, 0, sizeof(AkAllocFreeBlockHeader) - sizeof(AkAllocBlockHeader));
}

inline static AkI32 alloc_freeblock_height_of(const AkAllocFreeBlockHeader* n) noexcept { return n ? n->height : 0; }

inline static AkVoid alloc_freeblock_update(AkAllocFreeBlockHeader* n) noexcept {
    if (!n) return;
    const AkI32 hl = alloc_freeblock_height_of(n->left);
    const AkI32 hr = alloc_freeblock_height_of(n->right);
    n->height  = 1 + (hl > hr ? hl : hr);
    n->balance = hl - hr;
}

inline static AkVoid alloc_freeblock_rotate_left(AkAllocFreeBlockHeader** r, AkAllocFreeBlockHeader* x) noexcept {
    AkAllocFreeBlockHeader* y = x->right;
    AK_ASSERT(y != nullptr);
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
    alloc_freeblock_update(x);
    alloc_freeblock_update(y);
}

inline static AkVoid alloc_freeblock_rotate_right(AkAllocFreeBlockHeader** r, AkAllocFreeBlockHeader* y) noexcept {
    AkAllocFreeBlockHeader* x = y->left;
    AK_ASSERT(x != nullptr);
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
    alloc_freeblock_update(y);
    alloc_freeblock_update(x);
}

inline static AkVoid alloc_freeblock_rebalance_upwards(AkAllocFreeBlockHeader** r, AkAllocFreeBlockHeader* n) noexcept {
    while (n) {
        alloc_freeblock_update(n);
        if (n->balance > 1) {
            if (n->left && n->left->balance < 0) {
                alloc_freeblock_rotate_left(r, n->left);
            }
            alloc_freeblock_rotate_right(r, n);
        } else if (n->balance < -1) {
            if (n->right && n->right->balance > 0) {
                alloc_freeblock_rotate_right(r, n->right);
            }
            alloc_freeblock_rotate_left(r, n);
        }
        n = n->parent;
    }
}

inline static AkVoid alloc_freeblock_transplant(AkAllocFreeBlockHeader** r, AkAllocFreeBlockHeader* u, AkAllocFreeBlockHeader* v) noexcept {
    if (u->parent == nullptr) {
        *r = v;
    } else if (u->parent->left == u) {
        u->parent->left = v;
    } else {
        u->parent->right = v;
    }
    if (v) v->parent = u->parent;
}

inline static AkAllocFreeBlockHeader* alloc_freeblock_min_node(AkAllocFreeBlockHeader* n) noexcept {
    AK_ASSERT(n != nullptr);
    while (n->left) n = n->left;
    return n;
}
