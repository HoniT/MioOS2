// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Physical Memory Manager, Bump and Buddy allocators
// ========================================

#include <mm/pmm.hpp>
#include <lib/mem_util.hpp>
#include <kernel_panic.hpp>
#include <graphics/kprint.hpp>

using namespace mem;

extern "C" uint8_t kernel_start_phys[];
extern "C" uint8_t kernel_end_phys[];

#pragma region Bump Allocation

PhysAddr PMM::bump_ptr_phys = 0;
PhysAddr PMM::bump_region_end_phys = 0;
PhysAddr PMM::highest_reserved_phys = 0;
multiboot_tag_mmap* PMM::mmap = nullptr;

/// @brief Finds a usable memory region for bump allocations
void PMM::find_bump_alloc_region(size_t required_size) {
    uint8_t* mmap_start = (uint8_t*)mmap->entries;
    uint8_t* mmap_end   = (uint8_t*)mmap + mmap->size;
    usize entry_size = mmap->entry_size;

    for (uint8_t* ptr = mmap_start; ptr < mmap_end; ptr += entry_size) {
        multiboot_mmap_entry* ent = (multiboot_mmap_entry*)ptr;

        // Skip unavailable regions, the 0-page, and regions we've already exhausted
        if (ent->type != MULTIBOOT_MEMORY_AVAILABLE || ent->addr == 0 || ent->addr < bump_region_end_phys) continue;

        PhysAddr potential_start;

        // Check if our reserved data (Kernel + Multiboot Info) sits inside this region
        if (ent->addr <= highest_reserved_phys && ent->addr + ent->len > highest_reserved_phys) {
            potential_start = align_up(highest_reserved_phys, PAGE_SIZE);
        } else {
            potential_start = align_up(ent->addr, PAGE_SIZE);
        }

        if (potential_start + required_size <= ent->addr + ent->len) {
            bump_ptr_phys = potential_start;
            bump_region_end_phys = ent->addr + ent->len;
            return;
        }
    }

    kernel_panic("PMM", "No memory for bump allocation!\n");
}

/// @brief Allocates a page using the bump allocator (for early kernel use only)
/// @param num Number of pages to allocate
/// @return Page physical address
void* PMM::alloc_pages_bump(size_t num) {
    usize alloc_size = num * PAGE_SIZE;

    while (bump_ptr_phys == 0 || bump_ptr_phys + alloc_size > bump_region_end_phys) {
        find_bump_alloc_region(alloc_size);
    }

    uintptr_t allocated_phys = bump_ptr_phys;
    bump_ptr_phys += alloc_size;

    return (void*)(allocated_phys + HHDM_BASE);
}

bool PMM::initialize_bump(multiboot_tag_mmap* _mmap, void* multiboot_ptr) {
    if(_mmap == nullptr) {
        kernel_panic("PMM", "No mmap provided!\n");
        return false;
    }
    mmap = _mmap;

    // Calculate highest reserved address
    highest_reserved_phys = (PhysAddr)kernel_end_phys;
    
    // Calculate where the Multiboot tags end
    PhysAddr mb_total_size = *(VirtAddr*)multiboot_ptr - HIGHER_HALF_OFFSET;
    PhysAddr mb_end = (PhysAddr)multiboot_ptr + mb_total_size;

    if (mb_end > highest_reserved_phys) {
        highest_reserved_phys = mb_end;
    }

    klogf("PMM", "Initialized bump allocator (Highest reserved physical address: 0x%x)\n", highest_reserved_phys);
    return true;
}

#pragma endregion