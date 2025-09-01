#pragma once

#include "ak/base/base.hpp" // IWYU pragma: keep


struct AkFrameId 
{
    static constexpr AkU32 INVALID = 0;

    AkU32 id = INVALID;

    explicit AkFrameId(const AkU32& id) noexcept : id(id) {}
    AkFrameId() = default;
    AkFrameId(const AkFrameId& other) noexcept = default;
    AkFrameId(AkFrameId&& other) noexcept = default;
    AkFrameId& operator=(const AkFrameId& other) noexcept = default;
    AkFrameId& operator=(AkFrameId&& other) noexcept = default;

    constexpr operator AkBool() const noexcept { return id != 0; }

    constexpr AkBool operator==(const AkFrameId& other) const noexcept { return id == other.id; }
    constexpr AkBool operator!=(const AkFrameId& other) const noexcept { return id != other.id; }
    constexpr AkBool operator<(const AkFrameId& other) const noexcept { return id < other.id; }
    constexpr AkBool operator>(const AkFrameId& other) const noexcept { return id > other.id; }
    constexpr AkBool operator<=(const AkFrameId& other) const noexcept { return id <= other.id; }
    constexpr AkBool operator>=(const AkFrameId& other) const noexcept { return id >= other.id; }
};

struct AkPageId 
{
    static constexpr AkU32 INVALID = 0;

    AkU32 id = INVALID;

    explicit AkPageId(const AkU32& id) noexcept : id(id) {}
    AkPageId() noexcept = default;
    AkPageId(const AkPageId& other) noexcept = default;
    AkPageId(AkPageId&& other) noexcept = default;
    AkPageId& operator=(const AkPageId& other) noexcept = default;
    AkPageId& operator=(AkPageId&& other) noexcept = default;

    constexpr operator AkBool() const noexcept { return id != 0; }
    AkBool operator==(const AkPageId& other) const noexcept { return id == other.id; }
    AkBool operator!=(const AkPageId& other) const noexcept { return id != other.id; }
    AkBool operator<(const AkPageId& other) const noexcept { return id < other.id; }
    AkBool operator>(const AkPageId& other) const noexcept { return id > other.id; }
    AkBool operator<=(const AkPageId& other) const noexcept { return id <= other.id; }
    AkBool operator>=(const AkPageId& other) const noexcept { return id >= other.id; }
};

struct VPageId {
    static constexpr AkU32 INVALID = 0;

    AkU32 id = INVALID;

    explicit VPageId(const AkU32& id) noexcept : id(id) {}
    VPageId() noexcept = default;
    VPageId(const VPageId& other) noexcept = default;
    VPageId(VPageId&& other) noexcept = default;
    VPageId& operator=(const VPageId& other) noexcept = default;
    constexpr VPageId& operator=(VPageId&& other) noexcept = default;

    constexpr operator AkBool() const noexcept { return id != 0; }
    AkBool operator==(const VPageId& other) const noexcept { return id == other.id; }
    AkBool operator!=(const VPageId& other) const noexcept { return id != other.id; }
    AkBool operator<(const VPageId& other) const noexcept { return id < other.id; }
    AkBool operator>(const VPageId& other) const noexcept { return id > other.id; }
    AkBool operator<=(const VPageId& other) const noexcept { return id <= other.id; }
    AkBool operator>=(const VPageId& other) const noexcept { return id >= other.id; }
};

enum class AkBufferPool {
    INVALID = 0,
    DEFAULT,
    RECYCLE,
    KEEP
};
const char* ak_to_string(AkBufferPool p) noexcept;

struct AkFrameEntry {
    AkU32     pool      : 2;
    AkU32     is_dirty  : 1;
    AkU32     evict     : 1;
    AkU32     pin_count : 28;
    AkFrameId pool_index;
    AkPageId  page_cache_bucket;
    VPageId   vpage_cache_bucket;
};
static_assert(sizeof(AkFrameEntry) == 16, "FrameEntry must have a size of 16");

struct AkFramePool {
    AkFrameId* entries;
    AkU32      count;
    AkU32      capacity;
};

struct AkFrameTable {
    AkFrameEntry* entries;
    AkFramePool   free_pool;
    AkFramePool   default_pool;
    AkFramePool   recycle_pool;
    AkFramePool   keep_pool;
    AkU32         clock;
};

struct AkPagecacheEntry {
    AkPageId  page_id;
    AkFrameId frame_id;
};

struct AkPagecache {
    AkPagecacheEntry* entries = nullptr;
    AkU32 capacity = 0;
};

