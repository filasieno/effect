#pragma once

#include "ak/base/base.hpp" // IWYU pragma: keep

namespace ak 
{
    struct FrameId 
    {
        static constexpr U32 INVALID = 0;

        U32 id = INVALID;

        explicit FrameId(const U32& id) noexcept : id(id) {}
        FrameId() = default;
        FrameId(const FrameId& other) noexcept = default;
        FrameId(FrameId&& other) noexcept = default;
        FrameId& operator=(const FrameId& other) noexcept = default;
        FrameId& operator=(FrameId&& other) noexcept = default;

        constexpr operator Bool() const noexcept { return id != 0; }

        constexpr Bool operator==(const FrameId& other) const noexcept { return id == other.id; }
        constexpr Bool operator!=(const FrameId& other) const noexcept { return id != other.id; }
        constexpr Bool operator<(const FrameId& other) const noexcept { return id < other.id; }
        constexpr Bool operator>(const FrameId& other) const noexcept { return id > other.id; }
        constexpr Bool operator<=(const FrameId& other) const noexcept { return id <= other.id; }
        constexpr Bool operator>=(const FrameId& other) const noexcept { return id >= other.id; }
    };

    struct PageId 
    {
        static constexpr U32 INVALID = 0;

        U32 id = INVALID;

        explicit PageId(const U32& id) noexcept : id(id) {}
        PageId() noexcept = default;
        PageId(const PageId& other) noexcept = default;
        PageId(PageId&& other) noexcept = default;
        PageId& operator=(const PageId& other) noexcept = default;
        PageId& operator=(PageId&& other) noexcept = default;

        constexpr operator Bool() const noexcept { return id != 0; }
        Bool operator==(const PageId& other) const noexcept { return id == other.id; }
        Bool operator!=(const PageId& other) const noexcept { return id != other.id; }
        Bool operator<(const PageId& other) const noexcept { return id < other.id; }
        Bool operator>(const PageId& other) const noexcept { return id > other.id; }
        Bool operator<=(const PageId& other) const noexcept { return id <= other.id; }
        Bool operator>=(const PageId& other) const noexcept { return id >= other.id; }
    };

    struct VPageId {
        static constexpr U32 INVALID = 0;

        U32 id = INVALID;

        explicit VPageId(const U32& id) noexcept : id(id) {}
        VPageId() noexcept = default;
        VPageId(const VPageId& other) noexcept = default;
        VPageId(VPageId&& other) noexcept = default;
        VPageId& operator=(const VPageId& other) noexcept = default;
        constexpr VPageId& operator=(VPageId&& other) noexcept = default;

        constexpr operator Bool() const noexcept { return id != 0; }
        Bool operator==(const VPageId& other) const noexcept { return id == other.id; }
        Bool operator!=(const VPageId& other) const noexcept { return id != other.id; }
        Bool operator<(const VPageId& other) const noexcept { return id < other.id; }
        Bool operator>(const VPageId& other) const noexcept { return id > other.id; }
        Bool operator<=(const VPageId& other) const noexcept { return id <= other.id; }
        Bool operator>=(const VPageId& other) const noexcept { return id >= other.id; }
    };
}
