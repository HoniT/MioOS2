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
    // Intrusive doubly-linked list node placed at the start of free pages
    struct buddy_free_node {
        buddy_free_node* next;
        buddy_free_node* prev;
    };

    class PMM {
    private:
        static PhysAddr bump_ptr_phys;
        static PhysAddr bump_region_end_phys;
        static PhysAddr highest_reserved_phys;
        static multiboot_tag_mmap* mmap;

        static usize total_memory;
        static usize free_memory;
        static usize used_memory;

        // SMP Lock (TODO when I implement SMP)
        static void acquire_lock();
        static void release_lock();

        /// @brief Finds a usable memory region for bump allocations
        static void find_bump_alloc_region(size_t required_size);
        /// @brief Allocates a page using the bump allocator (for early kernel use only)
        /// @param num Number of pages to allocate
        /// @return Page physical address
        static void* alloc_pages_bump(size_t num = 1);

        static bool is_page_free(size_t target_pfn);
        static bool allocate_specific_page(size_t target_pfn);

    public:
        static bool initialized_buddy;

        /// @brief Initializes the PMM for early bump allocation
        /// @param mmap Multiboot2 mmap info
        static bool initialize_bump(multiboot_tag_mmap* mmap, void* multiboot_ptr);

        /// @brief Initializes the main buddy allocator for physical memory allocations
        /// @param mmap Memory map, defaults to PMM::mmap (set from initialize_bump) if not provided
        static bool initialize_buddy(multiboot_tag_mmap* mmap = mem::PMM::mmap);
    
    
        static void* alloc_pages(size_t num = 1);
        static void free_pages(void* p, size_t num = 1);

        /// @brief Marks a physical memore region as free
        /// @param base Physical address of the regions start
        /// @param length Length of the region
        static void mark_region_free(void* base, size_t length);
        /// @brief Marks a physical memore region as allocated
        /// @param base Physical address of the regions start
        /// @param length Length of the region
        static void mark_region_used(void* base, size_t length);

        // Statistics
        static size_t get_total_memory();
        static size_t get_free_memory();
        static size_t get_used_memory();
    };
} // namespace mem

#endif // PMM_HPP
