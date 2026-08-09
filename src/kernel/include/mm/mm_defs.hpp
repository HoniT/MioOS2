// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Memory management related definitions, constants
// ========================================

#pragma once
#ifndef MM_DEFS_HPP
#define MM_DEFS_HPP

#include <stdint.h>
#include <stddef.h>

namespace mem {
    constexpr const char* mem_regions_bios[5] = {
        "Available",
        "Reserved",
        "ACPI Reclaimable",
        "NVS",
        "Bad RAM"
    };

    constexpr const char* mem_regions_uefi[16] = {
        "EfiReservedMemoryType",
        "EfiLoaderCode",
        "EfiLoaderData",
        "EfiBootServicesCode",
        "EfiBootServicesData",
        "EfiRuntimeServicesCode",
        "EfiRuntimeServicesData",
        "EfiConventionalMemory",
        "EfiUnusableMemory",
        "EfiACPIReclaimMemory",
        "EfiACPIMemoryNVS",
        "EfiMemoryMappedIO",
        "EfiMemoryMappedIOPortSpace",
        "EfiPalCode",
        "EfiPersistentMemory",
        "EfiMaxMemoryType"
    };

    using PhysAddr = uint64_t;
    using VirtAddr = uint64_t;
    using usize = uint64_t;

    constexpr VirtAddr HHDM_BASE = 0xFFFF800000000000;
    constexpr PhysAddr EARLY_BOOT_MAP_LIMIT = 0x40000000;

    constexpr size_t BUDDY_MAX_ORDER = 10; // Max block size: PAGE_SIZE * 2^BUDDY_MAX_ORDER

    constexpr int NUM_KMALLOC_CACHES = 8;

    constexpr size_t PAGE_SHIFT   = 12;
    constexpr size_t PAGE_SIZE    = 1UL << PAGE_SHIFT;   // 4 KiB
    constexpr size_t PAGE_SIZE_2M = 1UL << 21;           // 2 MiB (huge)
    constexpr size_t PAGE_SIZE_1G = 1UL << 30;           // 1 GiB (huge)
    constexpr size_t PAGE_MASK    = PAGE_SIZE - 1;


    constexpr uint64_t PFN_MASK = 0x000F'FFFF'FFFF'F000ULL;
    constexpr usize INDEX_MASK  = 0x1FFU;   // 9-bit mask
    constexpr usize OFFSET_MASK = 0xFFFU;   // 12-bit mask
    constexpr usize PT_ENTRY_COUNT = 512;

    // Page permission flags
    enum class PageFlags : uint32_t {
        None         = 0,
        Read         = (1u << 0),
        Write        = (1u << 1),
        Execute      = (1u << 2),
        User         = (1u << 3),
        Global       = (1u << 4),
        WriteThrough = (1u << 5),
        NoCache      = (1u << 6),
        Huge2M       = (1u << 7),
        Huge1G       = (1u << 8),

        // Convenience presets
        KernelRO = Read,
        KernelRW = Read | Write,
        KernelRX = Read | Execute,
        KernelRWX= Read | Write | Execute,

        UserRO   = Read | User,
        UserRW   = Read | Write | User,
        UserRX   = Read | Execute | User,

        MMIO     = Read | Write | NoCache,
    };

    [[nodiscard]] constexpr PageFlags operator|(PageFlags a, PageFlags b) noexcept {
        return static_cast<PageFlags>(
            static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }
    [[nodiscard]] constexpr PageFlags operator&(PageFlags a, PageFlags b) noexcept {
        return static_cast<PageFlags>(
            static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }
    [[nodiscard]] constexpr PageFlags operator~(PageFlags a) noexcept {
        return static_cast<PageFlags>(~static_cast<uint32_t>(a));
    }
    constexpr PageFlags& operator|=(PageFlags& a, PageFlags b) noexcept {
        a = a | b; return a;
    }
    constexpr PageFlags& operator&=(PageFlags& a, PageFlags b) noexcept {
        a = a & b; return a;
    }


    [[nodiscard]] constexpr bool has_flag(PageFlags flags, PageFlags flag) noexcept {
        return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag))
            == static_cast<uint32_t>(flag);
    }

    /// Round `addr` UP to the next page boundary (or return as-is if aligned).
    [[nodiscard]] constexpr VirtAddr page_align_up(VirtAddr addr) noexcept {
        return (addr + PAGE_MASK) & ~static_cast<VirtAddr>(PAGE_MASK);
    }

    /// Round `addr` DOWN to the nearest page boundary.
    [[nodiscard]] constexpr VirtAddr page_align_down(VirtAddr addr) noexcept {
        return addr & ~static_cast<VirtAddr>(PAGE_MASK);
    }

    /// Return true iff `addr` is aligned to a page boundary.
    [[nodiscard]] constexpr bool is_page_aligned(VirtAddr addr) noexcept {
        return (addr & PAGE_MASK) == 0;
    }
} // namespace mem


// Placement new and delete for the kernel
inline void* operator new(size_t, void* p) noexcept { return p; }
inline void* operator new[](size_t, void* p) noexcept { return p; }
void* operator new(size_t size) = delete;
void* operator new[](size_t size) = delete;

#endif // MM_DEFS_HPP
