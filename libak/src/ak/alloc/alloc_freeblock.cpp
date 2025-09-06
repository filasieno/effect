#include "ak/alloc/alloc.hpp" // IWYU pragma: keep
#include <cstring>


// AVL utility forward declarations 

inline static int                     alloc_freeblock_height_of(const ak_alloc_free_block_header* n) noexcept;
inline static void                    alloc_freeblock_update(ak_alloc_free_block_header* n) noexcept;
inline static void                    alloc_freeblock_rotate_left(ak_alloc_free_block_header** root, ak_alloc_free_block_header* x) noexcept;
inline static void                    alloc_freeblock_rotate_right(ak_alloc_free_block_header** root, ak_alloc_free_block_header* y) noexcept;
inline static void                    alloc_freeblock_rebalance_upwards(ak_alloc_free_block_header** root, ak_alloc_free_block_header* n) noexcept;
inline static void                    alloc_freeblock_transplant(ak_alloc_free_block_header** root, ak_alloc_free_block_header* u, ak_alloc_free_block_header* v) noexcept;
inline static ak_alloc_free_block_header* alloc_freeblock_min_node(ak_alloc_free_block_header* root) noexcept;

void alloc_freeblock_init_root(ak_alloc_free_block_header** root) noexcept {
    AK_ASSERT(root != nullptr);
    *root = nullptr;
}

void alloc_freeblock_put(ak_alloc_free_block_header** root, ak_alloc_block_header* block) noexcept {
    AK_ASSERT(root != nullptr);
    AK_ASSERT(block != nullptr);
    AK_ASSERT(block->this_desc.state == (AkU32)AK_ALLOC_BLOCK_STATE_FREE);
    AK_ASSERT(block->this_desc.size > 2048);

    auto key_of = [](const ak_alloc_free_block_header* n) noexcept -> AkU64 { return n->this_desc.size; };
    // (helpers moved to static inline utilities above)

    ak_alloc_free_block_header* new_link = (ak_alloc_free_block_header*)block;

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
    ak_alloc_free_block_header* cur = *root;
    ak_alloc_free_block_header* parent = nullptr;
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

ak_alloc_free_block_header* alloc_freeblock_find_gte(ak_alloc_free_block_header* root, AkU64 block_size) noexcept {
    if (root == nullptr) return nullptr;
    if (block_size <= 2048) return nullptr;
    
    ak_alloc_free_block_header* node = root;
    ak_alloc_free_block_header* best = nullptr;
    while (node) {
        AkU64 k = node->this_desc.size;
        if (k == block_size) return node;
        if (k > block_size) { best = node; node = node->left; }
        else { node = node->right; }
    }
    return best;
}

void alloc_freeblock_detach(ak_alloc_free_block_header** root, ak_alloc_free_block_header* node) noexcept {
    AK_ASSERT(root != nullptr);
    AK_ASSERT(*root != nullptr);
    AK_ASSERT(node != nullptr);
    AK_ASSERT(node->this_desc.state == (AkU32)AK_ALLOC_BLOCK_STATE_FREE);
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
        ak_alloc_free_block_header* start_rebalance = node->parent;
        if (node->left == nullptr) {
            alloc_freeblock_transplant(root, node, node->right);
        } else if (node->right == nullptr) {
            alloc_freeblock_transplant(root, node, node->left);
        } else {
            ak_alloc_free_block_header* s = alloc_freeblock_min_node(node->right);
            if (s->parent != node) {
                // Replace s with its right subtree
                ak_alloc_free_block_header* sp = s->parent;
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
    
    ak_dlink* next_node_link = node->multimap_link.next;
    ak_alloc_free_block_header* next_node = (ak_alloc_free_block_header*)((char*)next_node_link - AK_OFFSET(ak_alloc_free_block_header, multimap_link));
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

bool alloc_freeblock_is_detached(const ak_alloc_free_block_header* link) noexcept {
    AK_ASSERT(link != nullptr);
    return link->multimap_link.next == &link->multimap_link && link->multimap_link.prev == &link->multimap_link;
}

// ------------------------------------------------------------------
// AVL utility implementations (moved to bottom for clarity)

void alloc_freeblock_clear(ak_alloc_free_block_header* link) noexcept {
    AK_ASSERT(link != nullptr);
    char* buff = ((char*)link) + sizeof(ak_alloc_block_header);
    std::memset(buff, 0, sizeof(ak_alloc_free_block_header) - sizeof(ak_alloc_block_header));
}

inline static int alloc_freeblock_height_of(const ak_alloc_free_block_header* n) noexcept { return n ? n->height : 0; }

inline static void alloc_freeblock_update(ak_alloc_free_block_header* n) noexcept {
    if (!n) return;
    const int hl = alloc_freeblock_height_of(n->left);
    const int hr = alloc_freeblock_height_of(n->right);
    n->height  = 1 + (hl > hr ? hl : hr);
    n->balance = hl - hr;
}

inline static void alloc_freeblock_rotate_left(ak_alloc_free_block_header** r, ak_alloc_free_block_header* x) noexcept {
    ak_alloc_free_block_header* y = x->right;
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

inline static void alloc_freeblock_rotate_right(ak_alloc_free_block_header** r, ak_alloc_free_block_header* y) noexcept {
    ak_alloc_free_block_header* x = y->left;
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

inline static void alloc_freeblock_rebalance_upwards(ak_alloc_free_block_header** r, ak_alloc_free_block_header* n) noexcept {
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

inline static void alloc_freeblock_transplant(ak_alloc_free_block_header** r, ak_alloc_free_block_header* u, ak_alloc_free_block_header* v) noexcept {
    if (u->parent == nullptr) {
        *r = v;
    } else if (u->parent->left == u) {
        u->parent->left = v;
    } else {
        u->parent->right = v;
    }
    if (v) v->parent = u->parent;
}

inline static ak_alloc_free_block_header* alloc_freeblock_min_node(ak_alloc_free_block_header* n) noexcept {
    AK_ASSERT(n != nullptr);
    while (n->left) n = n->left;
    return n;
}
