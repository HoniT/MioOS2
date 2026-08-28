// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// SLUB allocator unit tests
// ========================================

#include <tests/mm/slub_tests.hpp>
#include <mm/slub.hpp>
#include <mm/pmm.hpp>
#include <kernel_ui.hpp>
#include <kernel_panic.hpp>

using namespace mem;

#define SLUB_ASSERT(condition, msg) \
    if (!(condition)) { \
        kprintf(gui::LOG_ERROR, "%s\n", msg); \
        kernel_panic("Unit tests failed\n"); \
        return false; \
    }

bool mem::run_slub_tests() {
    // Test 1: Basic Allocation
    void* ptr = kmalloc(32);
    SLUB_ASSERT(ptr != nullptr, "kmalloc(32) returned nullptr");
    
    // Ensure pointer is mapped in the higher half
    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
    SLUB_ASSERT(addr >= HHDM_BASE, "Allocated pointer is not in higher half");
    
    kfree(ptr);

    // Test 2: Data Integrity
    size_t size = sizeof(int) * 10;
    int* arr = static_cast<int*>(kmalloc(size));
    SLUB_ASSERT(arr != nullptr, "kmalloc failed during data integrity test");

    // Write to memory
    for (int i = 0; i < 10; i++) {
        arr[i] = i * 100;
    }

    // Read back to ensure no page faults or data corruption occurred
    for (int i = 0; i < 10; i++) {
        SLUB_ASSERT(arr[i] == i * 100, "Data corruption detected in allocated memory");
    }
    
    kfree(arr);

    // Test 3: Slab Exhaustion and PMM Return
    size_t initial_free_memory = PMM::get_free_memory();
    
    // Allocate 128 objects of 64 bytes to force new physical page allocations
    void* objects[128];
    for (int i = 0; i < 128; i++) {
        objects[i] = kmalloc(64);
        SLUB_ASSERT(objects[i] != nullptr, "Failed to allocate object during stress test");
    }

    size_t memory_after_alloc = PMM::get_free_memory();
    SLUB_ASSERT(memory_after_alloc < initial_free_memory, "PMM free memory did not decrease");

    // Free all objects
    for (int i = 0; i < 128; i++) {
        kfree(objects[i]);
    }

    size_t memory_after_free = PMM::get_free_memory();
    SLUB_ASSERT(memory_after_free == initial_free_memory, "Memory leak: Empty slabs were not returned to PMM");

    // Test 4: Large Allocation (PMM Bypass)
    // Allocating exactly 1 page (4096 bytes) to bypass SLUB caches (max 2048)
    initial_free_memory = PMM::get_free_memory();
    
    void* large_ptr = kmalloc(PAGE_SIZE);
    SLUB_ASSERT(large_ptr != nullptr, "Large kmalloc failed");
    
    memory_after_alloc = PMM::get_free_memory();
    SLUB_ASSERT(initial_free_memory - memory_after_alloc >= PAGE_SIZE, "Large alloc did not pull page from PMM");

    kfree(large_ptr);

    memory_after_free = PMM::get_free_memory();
    SLUB_ASSERT(memory_after_free == initial_free_memory, "Large kfree did not return page to PMM");

    kprintf(gui::PrintTypes::LOG_INFO, "All SLUB tests passed successfully!\n");
    return true;
}

#undef SLUB_ASSERT