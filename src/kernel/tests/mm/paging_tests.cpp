// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Paging unit tests
// ========================================

#include <tests/mm/paging_tests.hpp>
#include <mm/paging.hpp>
#include <kernel_ui.hpp>
#include <kernel_panic.hpp>

using namespace mem;

#define VMM_ASSERT(condition, msg) \
    if (!(condition)) { \
        kprintf(gui::LOG_ERROR, "%s\n", msg); \
        kernel_panic("Unit tests failed\n"); \
        return false; \
    }

bool mem::run_paging_tests() {
    // Pick safe, page-aligned addresses away from kernel code and BIOS structures
    const VirtAddr test_virt = 0x20000000; // 512 MiB
    const PhysAddr test_phys = 0x30000000; // 768 MiB

    // Test 1: Alignment bounds checking
    PagingError err = PagingBackend::map_page(test_virt + 1, test_phys, PageFlags::KernelRW);
    VMM_ASSERT(err == PagingError::InvalidAlignment, "Failed to catch unaligned virtual address");

    err = PagingBackend::map_page(test_virt, test_phys + 0xFFF, PageFlags::KernelRW);
    VMM_ASSERT(err == PagingError::InvalidAlignment, "Failed to catch unaligned physical address");

    // Test 2: Standard Page Mapping
    err = PagingBackend::map_page(test_virt, test_phys, PageFlags::KernelRW);
    VMM_ASSERT(err == PagingError::Success, "map_page returned an error");

    PhysAddr translated = PagingBackend::translate(test_virt);
    VMM_ASSERT(translated == test_phys, "Translation mismatch after mapping");

    // Test 3: Hardware Memory Access
    // We use volatile so the compiler doesn't optimize away the physical RAM access
    volatile uint64_t* ptr = reinterpret_cast<uint64_t*>(test_virt);
    const uint64_t magic_value = 0xDEADBEEFCAFEBABE;
    
    *ptr = magic_value;
    VMM_ASSERT(*ptr == magic_value, "Hardware memory read/write failed");

    // Test 4: Duplicate Mapping Prevention
    err = PagingBackend::map_page(test_virt, test_phys, PageFlags::KernelRW);
    VMM_ASSERT(err == PagingError::AlreadyMapped, "VMM allowed overlapping map");

    // Test 5: Page Protection Updates
    // Switch the page to Read-Only
    err = PagingBackend::protect_page(test_virt, PageFlags::None); // Assuming 'None' removes writable bit
    VMM_ASSERT(err == PagingError::Success, "protect_page returned an error");
    
    translated = PagingBackend::translate(test_virt);
    VMM_ASSERT(translated == test_phys, "Translation lost after protect_page");
    
    // (We intentionally skip writing to 'ptr' here to avoid triggering an actual Page Fault)

    // Test 6: Page Unmapping
    err = PagingBackend::unmap_page(test_virt);
    VMM_ASSERT(err == PagingError::Success, "unmap_page returned an error");

    translated = PagingBackend::translate(test_virt);
    VMM_ASSERT(translated == 0, "Address still translates after unmap!");

    err = PagingBackend::unmap_page(test_virt);
    VMM_ASSERT(err == PagingError::NotMapped, "unmap_page didn't catch double unmap");

    kprintf(gui::PrintTypes::LOG_INFO, "All paging tests passed successfully!\n");
    return true;
}

#undef VMM_ASSERT
