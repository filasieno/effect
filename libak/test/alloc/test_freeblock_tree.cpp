#include <gtest/gtest.h>
#include <cstdlib>  // malloc, free
#include <vector>
#include <cstdint>
#include <algorithm>  // std::max

#include "ak/alloc/alloc.hpp" // IWYU pragma: keep

namespace {

struct MockBlock : public ak_alloc_free_block_header {
    MockBlock(AkU64 size) {
        this->this_desc.size = size;
        this->this_desc.state = (unsigned int)(AK_ALLOC_BLOCK_STATE_FREE);
    }
};

MockBlock* create_mock_block(AkU64 size) {
    void* mem = std::malloc(sizeof(MockBlock));
    if (!mem) {
        ADD_FAILURE() << "Malloc failed";
        return nullptr;
    }
    return new (mem) MockBlock(size);
}

bool is_tree_node(const ak_alloc_free_block_header* node) {
    return node->height >= 0;
}

int compute_height(const ak_alloc_free_block_header* node) {
    if (!node) return 0;
    if (!is_tree_node(node)) return 0;
    return 1 + std::max(compute_height(node->left), compute_height(node->right));
}

int compute_balance(const ak_alloc_free_block_header* node) {
    if (!node || !is_tree_node(node)) return 0;
    return compute_height(node->left) - compute_height(node->right);
}

void verify_tree(const ak_alloc_free_block_header* node, AkU64 min_key = 0, AkU64 max_key = UINT64_MAX) {
    if (!node) return;

    if (!is_tree_node(node)) {
        EXPECT_TRUE(alloc_freeblock_is_detached(node) || (node->multimap_link.next && node->multimap_link.prev));
        EXPECT_EQ(node->height, -1);
        EXPECT_EQ(node->balance, 0);
        EXPECT_EQ(node->left, nullptr);
        EXPECT_EQ(node->right, nullptr);
        EXPECT_EQ(node->parent, nullptr);
        return;
    }

    EXPECT_GE(node->height, 0);
    EXPECT_EQ(node->height, compute_height(node));
    EXPECT_EQ(node->balance, compute_balance(node));
    EXPECT_TRUE(node->balance >= -1 && node->balance <= 1) << "Unbalanced node size=" << node->this_desc.size;

    EXPECT_GT(node->this_desc.size, min_key);
    EXPECT_LT(node->this_desc.size, max_key);

    if (node->left) EXPECT_EQ(node->left->parent, node);
    if (node->right) EXPECT_EQ(node->right->parent, node);

    verify_tree(node->left, min_key, node->this_desc.size);
    verify_tree(node->right, node->this_desc.size, max_key);

    if (!alloc_freeblock_is_detached(node)) {
        const ak_alloc_free_block_header* current = node;
        int count = 0;
        do {
            EXPECT_EQ(current->this_desc.size, node->this_desc.size) << "List node size mismatch";
            if (!is_tree_node(current)) {
                verify_tree(current);
            }
            ak_dlink* nl = current->multimap_link.next;
            current = (const ak_alloc_free_block_header*)((const char*)nl - AK_OFFSET(ak_alloc_free_block_header, multimap_link));
            ++count;
            ASSERT_LT(count, 1000) << "Infinite list loop";
        } while (current != node);
    }
}

}  // namespace

TEST(AllocFreeBlockHeaderTest, Init) {
    ak_alloc_free_block_header* root = reinterpret_cast<ak_alloc_free_block_header*>(0xdeadbeef);
    alloc_freeblock_init_root(&root);
    EXPECT_EQ(root, nullptr);
}

TEST(AllocFreeBlockHeaderTest, InsertSingle) {
    std::vector<MockBlock*> blocks;
    ak_alloc_free_block_header* root = nullptr;

    MockBlock* b1 = create_mock_block(8192);
    blocks.push_back(b1);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b1));
    EXPECT_EQ(root, b1);
    EXPECT_EQ(b1->height, 1);
    EXPECT_EQ(b1->balance, 0);
    EXPECT_EQ(b1->parent, nullptr);
    EXPECT_EQ(b1->left, nullptr);
    EXPECT_EQ(b1->right, nullptr);
    EXPECT_EQ(b1->multimap_link.next, &b1->multimap_link);
    EXPECT_EQ(b1->multimap_link.prev, &b1->multimap_link);

    verify_tree(root);

    for (auto* b : blocks) {
        std::free(b);
    }
}

TEST(AllocFreeBlockHeaderTest, InsertDuplicate) {
    std::vector<MockBlock*> blocks;
    ak_alloc_free_block_header* root = nullptr;

    MockBlock* b1 = create_mock_block(8192);
    blocks.push_back(b1);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b1));

    MockBlock* b2 = create_mock_block(8192);
    blocks.push_back(b2);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b2));
    EXPECT_EQ(root, b1);
    EXPECT_EQ(b2->height, -1);
    EXPECT_EQ(b2->balance, 0);
    EXPECT_EQ(b2->parent, nullptr);
    EXPECT_EQ(b2->left, nullptr);
    EXPECT_EQ(b2->right, nullptr);
    EXPECT_EQ(b1->multimap_link.next, &b2->multimap_link);
    EXPECT_EQ(b2->multimap_link.next, &b1->multimap_link);
    EXPECT_EQ(b1->multimap_link.prev, &b2->multimap_link);
    EXPECT_EQ(b2->multimap_link.prev, &b1->multimap_link);

    verify_tree(root);
    EXPECT_FALSE(alloc_freeblock_is_detached(b1));
    EXPECT_FALSE(alloc_freeblock_is_detached(b2));

    for (auto* b : blocks) {
        std::free(b);
    }
}

TEST(AllocFreeBlockHeaderTest, InsertMultipleDuplicates) {
    std::vector<MockBlock*> blocks;
    ak_alloc_free_block_header* root = nullptr;

    MockBlock* b1 = create_mock_block(16384);
    blocks.push_back(b1);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b1));

    MockBlock* b2 = create_mock_block(16384);
    blocks.push_back(b2);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b2));

    MockBlock* b3 = create_mock_block(16384);
    blocks.push_back(b3);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b3));

    EXPECT_EQ(root, b1);
    EXPECT_EQ(b1->multimap_link.next, &b2->multimap_link);
    EXPECT_EQ(b2->multimap_link.next, &b3->multimap_link);
    EXPECT_EQ(b3->multimap_link.next, &b1->multimap_link);
    EXPECT_EQ(b1->multimap_link.prev, &b3->multimap_link);
    EXPECT_EQ(b3->multimap_link.prev, &b2->multimap_link);
    EXPECT_EQ(b2->multimap_link.prev, &b1->multimap_link);

    EXPECT_EQ(b1->height, 1);
    EXPECT_EQ(b2->height, -1);
    EXPECT_EQ(b3->height, -1);

    verify_tree(root);

    for (auto* b : blocks) {
        std::free(b);
    }
}

TEST(AllocFreeBlockHeaderTest, InsertRightRotation) {
    std::vector<MockBlock*> blocks;
    ak_alloc_free_block_header* root = nullptr;

    MockBlock* b300 = create_mock_block(24576);
    blocks.push_back(b300);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b300));

    MockBlock* b200 = create_mock_block(16384);
    blocks.push_back(b200);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b200));

    MockBlock* b100 = create_mock_block(8192);
    blocks.push_back(b100);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b100));

    EXPECT_EQ(root, b200);
    EXPECT_EQ(b200->left, b100);
    EXPECT_EQ(b200->right, b300);
    EXPECT_EQ(b100->parent, b200);
    EXPECT_EQ(b300->parent, b200);
    EXPECT_EQ(b200->height, 2);
    EXPECT_EQ(b200->balance, 0);

    verify_tree(root);

    for (auto* b : blocks) {
        std::free(b);
    }
}

TEST(AllocFreeBlockHeaderTest, InsertLeftRotation) {
    std::vector<MockBlock*> blocks;
    ak_alloc_free_block_header* root = nullptr;

    MockBlock* b100 = create_mock_block(8192);
    blocks.push_back(b100);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b100));

    MockBlock* b200 = create_mock_block(16384);
    blocks.push_back(b200);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b200));

    MockBlock* b300 = create_mock_block(24576);
    blocks.push_back(b300);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b300));

    EXPECT_EQ(root, b200);
    EXPECT_EQ(b200->left, b100);
    EXPECT_EQ(b200->right, b300);
    EXPECT_EQ(b100->parent, b200);
    EXPECT_EQ(b300->parent, b200);
    EXPECT_EQ(b200->height, 2);
    EXPECT_EQ(b200->balance, 0);

    verify_tree(root);

    for (auto* b : blocks) {
        std::free(b);
    }
}

TEST(AllocFreeBlockHeaderTest, InsertLeftRightRotation) {
    std::vector<MockBlock*> blocks;
    ak_alloc_free_block_header* root = nullptr;

    MockBlock* b300 = create_mock_block(24576);
    blocks.push_back(b300);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b300));

    MockBlock* b100 = create_mock_block(8192);
    blocks.push_back(b100);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b100));

    MockBlock* b200 = create_mock_block(16384);
    blocks.push_back(b200);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b200));

    EXPECT_EQ(root, b200);
    EXPECT_EQ(b200->left, b100);
    EXPECT_EQ(b200->right, b300);
    EXPECT_EQ(b100->parent, b200);
    EXPECT_EQ(b300->parent, b200);

    verify_tree(root);

    for (auto* b : blocks) {
        std::free(b);
    }
}

TEST(AllocFreeBlockHeaderTest, InsertRightLeftRotation) {
    std::vector<MockBlock*> blocks;
    ak_alloc_free_block_header* root = nullptr;

    MockBlock* b100 = create_mock_block(8192);
    blocks.push_back(b100);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b100));

    MockBlock* b300 = create_mock_block(24576);
    blocks.push_back(b300);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b300));

    MockBlock* b200 = create_mock_block(16384);
    blocks.push_back(b200);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b200));

    EXPECT_EQ(root, b200);
    EXPECT_EQ(b200->left, b100);
    EXPECT_EQ(b200->right, b300);

    verify_tree(root);

    for (auto* b : blocks) {
        std::free(b);
    }
}

TEST(AllocFreeBlockHeaderTest, FindGTE_Exact) {
    std::vector<MockBlock*> blocks;
    ak_alloc_free_block_header* root = nullptr;

    MockBlock* b64 = create_mock_block(8192);
    blocks.push_back(b64);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b64));

    MockBlock* b128 = create_mock_block(16384);
    blocks.push_back(b128);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b128));

    ak_alloc_free_block_header* found = alloc_freeblock_find_gte(root, 16384);
    EXPECT_EQ(found, b128);

    for (auto* b : blocks) {
        std::free(b);
    }
}

TEST(AllocFreeBlockHeaderTest, FindGTE_Greater) {
    std::vector<MockBlock*> blocks;
    ak_alloc_free_block_header* root = nullptr;

    MockBlock* b64 = create_mock_block(8192);
    blocks.push_back(b64);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b64));

    MockBlock* b256 = create_mock_block(32768);
    blocks.push_back(b256);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b256));

    ak_alloc_free_block_header* found = alloc_freeblock_find_gte(root, 16384);
    EXPECT_EQ(found, b256);

    for (auto* b : blocks) {
        std::free(b);
    }
}

TEST(AllocFreeBlockHeaderTest, FindGTE_None) {
    ak_alloc_free_block_header* root = nullptr;
    ak_alloc_free_block_header* found = alloc_freeblock_find_gte(root, 8192);
    EXPECT_EQ(found, nullptr);

    MockBlock* b32 = create_mock_block(16384);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b32));
    // Search strictly larger than any present so that none is found
    found = alloc_freeblock_find_gte(root, 65536);
    EXPECT_EQ(found, nullptr);
    std::free(b32);
}

TEST(AllocFreeBlockHeaderTest, DetachListNode) {
    std::vector<MockBlock*> blocks;
    ak_alloc_free_block_header* root = nullptr;

    MockBlock* b1 = create_mock_block(8192);
    blocks.push_back(b1);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b1));

    MockBlock* b2 = create_mock_block(8192);
    blocks.push_back(b2);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b2));

    MockBlock* b3 = create_mock_block(8192);
    blocks.push_back(b3);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b3));

    alloc_freeblock_detach(&root, b2);
    // List-node detach clears the node
    EXPECT_EQ(b2->multimap_link.next, nullptr);
    EXPECT_EQ(b2->multimap_link.prev, nullptr);
    EXPECT_EQ(b2->left, nullptr);
    EXPECT_EQ(b2->right, nullptr);

    EXPECT_EQ(b1->multimap_link.next, &b3->multimap_link);
    EXPECT_EQ(b3->multimap_link.next, &b1->multimap_link);
    EXPECT_EQ(b1->multimap_link.prev, &b3->multimap_link);
    EXPECT_EQ(b3->multimap_link.prev, &b1->multimap_link);

    verify_tree(root);

    for (auto* b : blocks) {
        std::free(b);
    }
}

TEST(AllocFreeBlockHeaderTest, DetachTreeNodeLeaf) {
    std::vector<MockBlock*> blocks;
    ak_alloc_free_block_header* root = nullptr;

    MockBlock* b100 = create_mock_block(8192);
    blocks.push_back(b100);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b100));

    alloc_freeblock_detach(&root, b100);
    EXPECT_EQ(root, nullptr);
    // Tree-node detach clears the node
    EXPECT_EQ(b100->multimap_link.next, nullptr);
    EXPECT_EQ(b100->multimap_link.prev, nullptr);
    EXPECT_EQ(b100->left, nullptr);
    EXPECT_EQ(b100->right, nullptr);

    verify_tree(root);

    for (auto* b : blocks) {
        std::free(b);
    }
}

TEST(AllocFreeBlockHeaderTest, DetachTreeNodeOneChild) {
    std::vector<MockBlock*> blocks;
    ak_alloc_free_block_header* root = nullptr;

    MockBlock* b100 = create_mock_block(8192);
    blocks.push_back(b100);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b100));

    MockBlock* b50 = create_mock_block(16384);
    blocks.push_back(b50);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b50));

    alloc_freeblock_detach(&root, b100);
    EXPECT_EQ(root, b50);
    EXPECT_EQ(b50->parent, nullptr);
    // Detached node cleared
    EXPECT_EQ(b100->multimap_link.next, nullptr);
    EXPECT_EQ(b100->multimap_link.prev, nullptr);

    verify_tree(root);

    for (auto* b : blocks) {
        std::free(b);
    }
}

TEST(AllocFreeBlockHeaderTest, DetachTreeNodeTwoChildren) {
    std::vector<MockBlock*> blocks;
    ak_alloc_free_block_header* root = nullptr;

    MockBlock* b100 = create_mock_block(8192);
    blocks.push_back(b100);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b100));

    MockBlock* b50 = create_mock_block(16384);
    blocks.push_back(b50);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b50));

    MockBlock* b150 = create_mock_block(24576);
    blocks.push_back(b150);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b150));

    MockBlock* b125 = create_mock_block(20480);
    blocks.push_back(b125);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b125));

    alloc_freeblock_detach(&root, b100);
    EXPECT_EQ(root, b125);
    EXPECT_EQ(b125->left, b50);
    EXPECT_EQ(b125->right, b150);
    EXPECT_EQ(b50->parent, b125);
    EXPECT_EQ(b150->parent, b125);

    verify_tree(root);

    for (auto* b : blocks) {
        std::free(b);
    }
}

TEST(AllocFreeBlockHeaderTest, DetachHeadWithList) {
    std::vector<MockBlock*> blocks;
    ak_alloc_free_block_header* root = nullptr;

    MockBlock* b1 = create_mock_block(8192);
    blocks.push_back(b1);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b1));

    MockBlock* b2 = create_mock_block(8192);
    blocks.push_back(b2);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b2));

    MockBlock* b3 = create_mock_block(8192);
    blocks.push_back(b3);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b3));

    alloc_freeblock_detach(&root, b1);
    EXPECT_EQ(root, b2);
    EXPECT_TRUE(is_tree_node(b2));
    EXPECT_EQ(b2->height, 1);
    EXPECT_EQ(b2->multimap_link.next, &b3->multimap_link);
    EXPECT_EQ(b3->multimap_link.next, &b2->multimap_link);
    EXPECT_EQ(b2->multimap_link.prev, &b3->multimap_link);
    EXPECT_EQ(b3->multimap_link.prev, &b2->multimap_link);
    // Head-node detach clears the old head
    EXPECT_EQ(b1->multimap_link.next, nullptr);
    EXPECT_EQ(b1->multimap_link.prev, nullptr);

    verify_tree(root);

    for (auto* b : blocks) {
        std::free(b);
    }
}

TEST(AllocFreeBlockHeaderTest, DetachLastInListPromotes) {
    std::vector<MockBlock*> blocks;
    ak_alloc_free_block_header* root = nullptr;

    MockBlock* b1 = create_mock_block(8192);
    blocks.push_back(b1);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b1));

    MockBlock* b2 = create_mock_block(8192);
    blocks.push_back(b2);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b2));

    alloc_freeblock_detach(&root, b2);

    alloc_freeblock_detach(&root, b1);
    EXPECT_EQ(root, nullptr);

    for (auto* b : blocks) {
        std::free(b);
    }
}

TEST(AllocFreeBlockHeaderTest, FIFOOrder) {
    std::vector<MockBlock*> blocks;
    ak_alloc_free_block_header* root = nullptr;

    MockBlock* b1 = create_mock_block(8192);
    blocks.push_back(b1);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b1));

    MockBlock* b2 = create_mock_block(8192);
    blocks.push_back(b2);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b2));

    MockBlock* b3 = create_mock_block(8192);
    blocks.push_back(b3);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b3));

    ak_alloc_free_block_header* group = alloc_freeblock_find_gte(root, 8192);
    EXPECT_EQ(group, b1);

    alloc_freeblock_detach(&root, group);
    
    EXPECT_EQ(root, b2);
    EXPECT_EQ(b2->multimap_link.next, &b3->multimap_link);

    group = alloc_freeblock_find_gte(root, 8192);
    alloc_freeblock_detach(&root, group);
    
    EXPECT_EQ(root, b3);
    EXPECT_TRUE(alloc_freeblock_is_detached(b3));

    group = alloc_freeblock_find_gte(root, 8192);
    alloc_freeblock_detach(&root, group);
    
    EXPECT_EQ(root, nullptr);

    for (auto* b : blocks) {
        std::free(b);
    }
}

TEST(AllocFreeBlockHeaderTest, Clear) {
    MockBlock* b = create_mock_block(8192);
    b->height = 5;
    b->balance = 2;
    b->parent = reinterpret_cast<ak_alloc_free_block_header*>(0x1);
    b->left = reinterpret_cast<ak_alloc_free_block_header*>(0x2);
    b->right = reinterpret_cast<ak_alloc_free_block_header*>(0x3);
    b->multimap_link.next = &b->multimap_link;
    b->multimap_link.prev = &b->multimap_link;

    alloc_freeblock_clear(b);

    EXPECT_EQ(b->height, 0);
    EXPECT_EQ(b->balance, 0);
    EXPECT_EQ(b->parent, nullptr);
    EXPECT_EQ(b->left, nullptr);
    EXPECT_EQ(b->right, nullptr);
    EXPECT_EQ(b->multimap_link.next, nullptr);
    EXPECT_EQ(b->multimap_link.prev, nullptr);

    EXPECT_EQ((AkU64)b->this_desc.size, (AkU64)8192);
    EXPECT_EQ(b->this_desc.state, (unsigned int)(AK_ALLOC_BLOCK_STATE_FREE));

    std::free(b);
}

TEST(AllocFreeBlockHeaderTest, IsDetached) {
    MockBlock* b = create_mock_block(8192);
    b->multimap_link.next = &b->multimap_link;
    b->multimap_link.prev = &b->multimap_link;
    EXPECT_TRUE(alloc_freeblock_is_detached(b));

    b->multimap_link.next = reinterpret_cast<ak_dlink*>(0x1);
    EXPECT_FALSE(alloc_freeblock_is_detached(b));

    b->multimap_link.prev = reinterpret_cast<ak_dlink*>(0x2);
    EXPECT_FALSE(alloc_freeblock_is_detached(b));

    b->multimap_link.next = &b->multimap_link;
    b->multimap_link.prev = &b->multimap_link;
    EXPECT_TRUE(alloc_freeblock_is_detached(b));

    std::free(b);
}

TEST(AllocFreeBlockHeaderTest, DetachTriggersRightRotation) {
    std::vector<MockBlock*> blocks;
    ak_alloc_free_block_header* root = nullptr;

    MockBlock* b4 = create_mock_block(32768);
    blocks.push_back(b4);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b4));

    MockBlock* b2 = create_mock_block(16384);
    blocks.push_back(b2);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b2));

    MockBlock* b5 = create_mock_block(40960);
    blocks.push_back(b5);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b5));

    MockBlock* b1 = create_mock_block(8192);
    blocks.push_back(b1);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b1));

    MockBlock* b3 = create_mock_block(24576);
    blocks.push_back(b3);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b3));

    EXPECT_EQ(root, b4);
    EXPECT_EQ(b4->left, b2);
    EXPECT_EQ(b4->right, b5);
    EXPECT_EQ(b2->left, b1);
    EXPECT_EQ(b2->right, b3);

    alloc_freeblock_detach(&root, b5);
    
    EXPECT_EQ(root, b2);
    EXPECT_EQ(b2->left, b1);
    EXPECT_EQ(b2->right, b4);
    EXPECT_EQ(b4->left, b3);
    EXPECT_EQ(b4->right, nullptr);
    EXPECT_EQ(b3->parent, b4);
    EXPECT_EQ(b4->parent, b2);

    verify_tree(root);

    for (auto* b : blocks) {
        std::free(b);
    }
}

TEST(AllocFreeBlockHeaderTest, DetachTriggersLeftRotation) {
    std::vector<MockBlock*> blocks;
    ak_alloc_free_block_header* root = nullptr;

    // Build tree with unique keys: root=16384, left=8192, right=24576, right-left=20480, right-right=32768
    MockBlock* b1 = create_mock_block(16384);
    blocks.push_back(b1);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b1));

    MockBlock* b3 = create_mock_block(24576);
    blocks.push_back(b3);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b3));

    MockBlock* b0 = create_mock_block(8192);
    blocks.push_back(b0);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b0));

    MockBlock* b2 = create_mock_block(20480);
    blocks.push_back(b2);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b2));

    MockBlock* b4 = create_mock_block(32768);
    blocks.push_back(b4);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b4));

    EXPECT_EQ(root, b1);
    EXPECT_EQ(b1->left, b0);
    EXPECT_EQ(b1->right, b3);
    EXPECT_EQ(b3->left, b2);
    EXPECT_EQ(b3->right, b4);

    alloc_freeblock_detach(&root, b0);
    
    EXPECT_EQ(root, b3);
    EXPECT_EQ(b3->left, b1);
    EXPECT_EQ(b3->right, b4);
    EXPECT_EQ(b1->right, b2);
    EXPECT_EQ(b1->left, nullptr);
    EXPECT_EQ(b2->parent, b1);
    EXPECT_EQ(b1->parent, b3);

    verify_tree(root);

    for (auto* b : blocks) {
        std::free(b);
    }
}

TEST(AllocFreeBlockHeaderTest, LargeTreeMultipleOperations) {
    std::vector<MockBlock*> blocks;
    ak_alloc_free_block_header* root = nullptr;

    std::vector<AkU64> sizes = {8192, 16384, 24576, 16384, 24576, 32768, 40960, 16384, 24576, 8192, 8192, 24576};

    for (AkU64 s : sizes) {
        MockBlock* b = create_mock_block(s);
        blocks.push_back(b);
        alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b));
        verify_tree(root);
    }

    for (size_t i = 0; i < blocks.size() / 2; ++i) {
        ak_alloc_free_block_header* to_detach = alloc_freeblock_find_gte(root, 16384);
        if (to_detach) {
            alloc_freeblock_detach(&root, to_detach);
            verify_tree(root);
        }
    }

    while (root) {
        ak_alloc_free_block_header* to_detach = root;
        alloc_freeblock_detach(&root, to_detach);
        verify_tree(root);
    }

    for (auto* b : blocks) {
        std::free(b);
    }
}

TEST(AllocFreeBlockHeaderTest, MinSize) {
    ak_alloc_free_block_header* root = nullptr;
    MockBlock* b = create_mock_block(8192);
    alloc_freeblock_put(&root, reinterpret_cast<ak_alloc_block_header*>(b));
    verify_tree(root);
    alloc_freeblock_detach(&root, b);
    std::free(b);
}
