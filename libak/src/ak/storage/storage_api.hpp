#pragma once

#include "ak/base/base.hpp" // IWYU pragma: keep


struct ak_frame_id 
{
    static constexpr unsigned int INVALID = 0;

    AkU32 id = INVALID;

    explicit ak_frame_id(const AkU32& id) noexcept : id(id) {}
    ak_frame_id() = default;
    ak_frame_id(const ak_frame_id& other) noexcept = default;
    ak_frame_id(ak_frame_id&& other) noexcept = default;
    ak_frame_id& operator=(const ak_frame_id& other) noexcept = default;
    ak_frame_id& operator=(ak_frame_id&& other) noexcept = default;

    constexpr operator bool() const noexcept { return id != 0; }

    constexpr bool operator==(const ak_frame_id& other) const noexcept { return id == other.id; }
    constexpr bool operator!=(const ak_frame_id& other) const noexcept { return id != other.id; }
    constexpr bool operator<(const ak_frame_id& other) const noexcept { return id < other.id; }
    constexpr bool operator>(const ak_frame_id& other) const noexcept { return id > other.id; }
    constexpr bool operator<=(const ak_frame_id& other) const noexcept { return id <= other.id; }
    constexpr bool operator>=(const ak_frame_id& other) const noexcept { return id >= other.id; }
};

struct ak_page_id 
{
    static constexpr AkU32 INVALID = 0;

    AkU32 id = INVALID;

    explicit ak_page_id(const AkU32& id) noexcept : id(id) {}
    ak_page_id() noexcept = default;
    ak_page_id(const ak_page_id& other) noexcept = default;
    ak_page_id(ak_page_id&& other) noexcept = default;
    ak_page_id& operator=(const ak_page_id& other) noexcept = default;
    ak_page_id& operator=(ak_page_id&& other) noexcept = default;

    constexpr operator bool() const noexcept { return id != 0; }
    bool operator==(const ak_page_id& other) const noexcept { return id == other.id; }
    bool operator!=(const ak_page_id& other) const noexcept { return id != other.id; }
    bool operator<(const ak_page_id& other) const noexcept { return id < other.id; }
    bool operator>(const ak_page_id& other) const noexcept { return id > other.id; }
    bool operator<=(const ak_page_id& other) const noexcept { return id <= other.id; }
    bool operator>=(const ak_page_id& other) const noexcept { return id >= other.id; }
};

struct ak_vpage_id {
    static constexpr AkU32 INVALID = 0;

    AkU32 id = INVALID;

    explicit ak_vpage_id(const AkU32& id) noexcept : id(id) {}
    ak_vpage_id() noexcept = default;
    ak_vpage_id(const ak_vpage_id& other) noexcept = default;
    ak_vpage_id(ak_vpage_id&& other) noexcept = default;
    ak_vpage_id& operator=(const ak_vpage_id& other) noexcept = default;
    constexpr ak_vpage_id& operator=(ak_vpage_id&& other) noexcept = default;

    constexpr operator bool() const noexcept { return id != 0; }
    bool operator==(const ak_vpage_id& other) const noexcept { return id == other.id; }
    bool operator!=(const ak_vpage_id& other) const noexcept { return id != other.id; }
    bool operator<(const ak_vpage_id& other) const noexcept { return id < other.id; }
    bool operator>(const ak_vpage_id& other) const noexcept { return id > other.id; }
    bool operator<=(const ak_vpage_id& other) const noexcept { return id <= other.id; }
    bool operator>=(const ak_vpage_id& other) const noexcept { return id >= other.id; }
};

enum ak_bufferpool {
    AK_BUFFERPOOL_INVALID = 0,
    AK_BUFFERPOOL_DEFAULT,
    AK_BUFFERPOOL_RECYCLE,
    AK_BUFFERPOOL_KEEP
};
const char* ak_to_string(ak_bufferpool p) noexcept;

struct ak_framepool_entry {
    AkU32       pool      : 2;
    AkU32       is_dirty  : 1;
    AkU32       evict     : 1;
    AkU32       pin_count : 28;
    ak_frame_id pool_index;
    ak_page_id  page_cache_bucket;
    ak_vpage_id vpage_cache_bucket;
};
static_assert(sizeof(struct ak_framepool_entry) == 16, "FrameEntry must have a size of 16");

struct ak_framepool {
    struct ak_frame_id* entries;
    AkU32      count;
    AkU32      capacity;
};

struct ak_frametable {
    struct ak_framepool_entry* entries;

    struct ak_framepool free_pool;
    struct ak_framepool default_pool;
    struct ak_framepool recycle_pool;
    struct ak_framepool keep_pool;
    AkU32               clock;
};

struct ak_pagecahe_entry {
    ak_page_id  page_id;
    ak_frame_id frame_id;
};

struct ak_pagecache {
    struct ak_pagecahe_entry* entries  = nullptr;
    AkU32                     capacity = 0;
};

