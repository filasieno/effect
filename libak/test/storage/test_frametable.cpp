#include <gtest/gtest.h>

#include <ak/storage/storage.hpp>


TEST(FrameTable, empty) {
}

// TEST(FrameTable, Init) {
//     void *test_mem = std::malloc(TEST_MEM_SIZE);
//     ASSERT_NE(test_mem, nullptr);

//     AllocTable at;
//     priv::init_alloc_table(&at, test_mem, TEST_MEM_SIZE);

//     FrameTable ft;
//     init_frame_table(&ft, 4, &at);

//     FrameId frame_01 = allocate_frame(&ft, BufferPool::KEEP);
//     FrameId frame_02 = allocate_frame(&ft, BufferPool::RECYCLE);
//     FrameId frame_03 = allocate_frame(&ft, BufferPool::DEFAULT);
//     FrameId frame_04 = allocate_frame(&ft, BufferPool::DEFAULT);

//     EXPECT_EQ(0u, get_frame_table_free_count(&ft));

//     EXPECT_EQ(0u, ft.free_pool.count);
//     EXPECT_EQ(2u, ft.default_pool.count);
//     EXPECT_EQ(1u, ft.keep_pool.count);
//     EXPECT_EQ(1u, ft.recycle_pool.count);

//     move_frame_to_pool(&ft, frame_03, BufferPool::KEEP);
//     move_frame_to_pool(&ft, frame_04, BufferPool::KEEP);
//     move_frame_to_pool(&ft, frame_02, BufferPool::KEEP);

//     EXPECT_EQ(0u, ft.free_pool.count);
//     EXPECT_EQ(0u, ft.default_pool.count);
//     EXPECT_EQ(4u, ft.keep_pool.count);
//     EXPECT_EQ(0u, ft.recycle_pool.count);

//     fini_frame_table(&ft, &at);
//     std::free(test_mem);
// }
