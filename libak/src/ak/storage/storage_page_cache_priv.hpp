#pragma once

#include "ak/storage/storage_api.hpp" // IWYU pragma: keep
#include "ak/alloc/alloc.hpp"         // IWYU pragma: keep

namespace ak {

    struct PagecacheEntry {
        PageId  page_id;
        FrameId frame_id;
    };

    struct Pagecache {
        PagecacheEntry* entries = nullptr;
        AkU32 capacity = 0;
    };

    Pagecache init_pagecache(AkAllocTable* at, AkU32 capacity) noexcept;
    void      fini_pagecache(Pagecache* cache, AkAllocTable* at) noexcept;

    AkBool    contains_pagecache_entry(const Pagecache* cache, PageId page_id) noexcept;
    FrameId   lookup_pagecahe_entry(const Pagecache* cache, PageId page_id) noexcept;
    
    AkU32     put_pagecache_entry(Pagecache* cache, PageId page_id, FrameId frame_id) noexcept;
    FrameId   remove_pagecache_entry(Pagecache* cache, PageId page_id) noexcept;

} // namespace ak
