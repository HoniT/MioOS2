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
    constexpr const char* mem_regions[5] = {
        "Available",
        "Reserved",
        "ACPI Reclaimable",
        "NVS",
        "Bad RAM"
    };

    using PhysAddr = uint64_t;
    using VirtAddr = uint64_t;
    using usize = uint64_t;

    constexpr PhysAddr HIGHER_HALF_OFFSET = 0xFFFFFFFF80000000;
    constexpr VirtAddr HHDM_BASE = 0xFFFF800000000000;

    constexpr size_t PAGE_SHIFT   = 12;
    constexpr size_t PAGE_SIZE    = 1UL << PAGE_SHIFT;   // 4 KiB
    constexpr size_t PAGE_SIZE_2M = 1UL << 21;           // 2 MiB (huge)
    constexpr size_t PAGE_SIZE_1G = 1UL << 30;           // 1 GiB (huge)
    constexpr size_t PAGE_MASK    = PAGE_SIZE - 1;
} // namespace mem

#endif // MM_DEFS_HPP
