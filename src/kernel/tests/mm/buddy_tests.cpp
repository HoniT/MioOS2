// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Unit tests for PMM buddy allocator
// ========================================

#include <tests/mm/buddy_tests.hpp>
#include <mm/pmm.hpp>
#include <graphics/kernel_gui.hpp>
#include <kernel_panic.hpp>

#define PMM_ASSERT(condition, msg) \
    if (!(condition)) { \
        kprintf(gui::LOG_ERROR, "%s\n", msg); \
        kernel_panic("Unit tests failed\n"); \
        return false; \
    }

bool mem::run_buddy_tests() {
    if (!PMM::initialized_buddy) {
        kprintf(gui::LOG_WARNING, "PMM test Aborted: PMM is not initialized!\n");
        return false;
    }

    size_t initial_free = PMM::get_free_memory();
    size_t initial_used = PMM::get_used_memory();

    // 1. Test basic allocation and freeing
    void* p1 = PMM::alloc_pages();
    void* p2 = PMM::alloc_pages(2);
    
    PMM_ASSERT(p1 && p2, "Basic allocation returned nullptr\n");

    PMM::free_pages(p1);
    PMM::free_pages(p2, 2);

    PMM_ASSERT(PMM::get_free_memory() == initial_free && PMM::get_used_memory() == initial_used,
        "Memory leak detected after basic free!\n");

    // 2. Test buddy merging (fragmentation check)
    // Allocate a block of order 3 (8 pages)
    void* block = PMM::alloc_pages(8);
    void* p_tmp = PMM::alloc_pages(); // Break alignment
    PMM::free_pages(p_tmp);
    
    PMM::free_pages(block, 8);
    
    PMM_ASSERT(PMM::get_free_memory() == initial_free, "Buddy system failed to fully merge blocks\n");

    // 3. Test region marking
    size_t region_size = PAGE_SIZE * 4;
    void* region_ptr = PMM::alloc_pages(4);
    PMM::free_pages(region_ptr, 4);
    
    PMM::mark_region_used(region_ptr, region_size);
    PMM_ASSERT(PMM::get_used_memory() == initial_used + region_size, "mark_region_used did not account for memory\n");

    PMM::mark_region_free(region_ptr, region_size);
    PMM_ASSERT(PMM::get_free_memory() == initial_free, "mark_region_free did not restore memory\n");

    kprintf(gui::LOG_INFO, "All buddy allocator tests passed successfully!\n");
    return true;
}