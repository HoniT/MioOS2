// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef PMM_HPP
#define PMM_HPP

#include <multiboot.hpp>
#include <mm/mm_defs.hpp>

namespace mem {
    class PMM {
    private:
        static PhysAddr bump_ptr_phys;
        static PhysAddr bump_region_end_phys;
        static PhysAddr highest_reserved_phys;
        static multiboot_tag_mmap* mmap;

        /// @brief Finds a usable memory region for bump allocations
        static void find_bump_alloc_region(size_t required_size);

        static bool initialized_buddy;
    public:
        /// @brief Initializes the PMM for early bump allocation
        /// @param mmap Multiboot2 mmap info
        static bool initialize_bump(multiboot_tag_mmap* mmap, void* multiboot_ptr);
    
        /// @brief Allocates a page using the bump allocator (for early kernel use only)
        /// @param num Number of pages to allocate
        /// @return Page physical address
        static void* alloc_pages_bump(size_t num = 1);
    
        static void* alloc_pages(size_t num = 1);
        static void free_pages(size_t num = 1);
    };
} // namespace mem

#endif // PMM_HPP
