// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Physical Memory Manager, Bump and Buddy allocators
// ========================================

#include <mm/pmm.hpp>
#include <mm/paging.hpp>
#include <mm/mmap_util.hpp>
#include <lib/mem_util.hpp>
#include <kernel_panic.hpp>
#include <kernel_ui.hpp>

using namespace mem;

extern "C" uint8_t kernel_start_phys[];
extern "C" uint8_t kernel_end_phys[];

bool PMM::initialized_buddy = false;
multiboot_tag* PMM::mmap = nullptr;
usize PMM::total_memory = 0;
usize PMM::free_memory = 0;
usize PMM::used_memory = 0;

#pragma region Bump Allocation

PhysAddr PMM::bump_ptr_phys = 0;
PhysAddr PMM::bump_region_end_phys = 0;
PhysAddr PMM::highest_reserved_phys = 0;

static PhysAddr bump_watermark;

/// @brief Finds a usable memory region for bump allocations
void PMM::find_bump_alloc_region(size_t required_size) {
    bool region_found = false;
    MmapIterator iter(mmap);

    iter.for_each([&](const UnifiedMemoryEntry& ent) {
        if (region_found) return;
        if (!ent.is_usable || ent.addr == 0) return;

        PhysAddr entry_end = ent.addr + ent.len;
        if (entry_end <= bump_watermark) return; // entirely consumed/behind us, skip

        PhysAddr potential_start = align_up(
            (ent.addr > bump_watermark) ? ent.addr : bump_watermark,
            PAGE_SIZE
        );

        if (potential_start + required_size > EARLY_BOOT_MAP_LIMIT) return;

        PhysAddr region_end = entry_end;
        if (potential_start + required_size <= region_end) {
            bump_ptr_phys = potential_start;
            bump_region_end_phys = (region_end > EARLY_BOOT_MAP_LIMIT) ? EARLY_BOOT_MAP_LIMIT : region_end;
            region_found = true;
        }
    });

    if (!region_found) {
        kernel_panic("No memory for bump allocation!\n");
    }
}

/// @brief Allocates a page using the bump allocator (for early kernel use only)
/// @param num Number of pages to allocate
/// @return Page physical address
void* PMM::alloc_pages_bump(size_t num) {
    if(initialized_buddy) return nullptr;

    usize alloc_size = num * PAGE_SIZE;

    while (bump_ptr_phys == 0 || bump_ptr_phys + alloc_size > bump_region_end_phys) {
        find_bump_alloc_region(alloc_size);
    }

    uintptr_t allocated_phys = bump_ptr_phys;
    bump_ptr_phys += alloc_size;

    return (void*)(allocated_phys);
}

bool PMM::initialize_bump(multiboot_tag* _mmap, void* multiboot_ptr) {
    if(_mmap == nullptr) {
        kernel_panic("No mmap provided!\n");
        return false;
    }
    mmap = _mmap;

    // Calculate highest reserved address
    highest_reserved_phys = (PhysAddr)kernel_end_phys;
    
    // Calculate where the Multiboot tags end
    PhysAddr mb_total_size = *(VirtAddr*)multiboot_ptr - HHDM_BASE;
    PhysAddr mb_end = (PhysAddr)multiboot_ptr + mb_total_size;

    if (mb_end > highest_reserved_phys) {
        highest_reserved_phys = mb_end;
    }
    bump_watermark = highest_reserved_phys;

    kprintf(gui::PrintTypes::LOG_INFO, "Initialized bump allocator (Highest reserved physical address: 0x%x)\n", highest_reserved_phys);
    return true;
}

#pragma endregion


static buddy_free_node* free_lists[BUDDY_MAX_ORDER + 1];
static uint8_t* buddy_map[BUDDY_MAX_ORDER + 1];

#pragma region Buddy Helpers

// Adds a block to a specific order's free list
static void list_add(buddy_free_node* block, size_t order) {
    block->prev = nullptr;
    block->next = free_lists[order];
    if (free_lists[order]) {
        free_lists[order]->prev = block;
    }
    free_lists[order] = block;
}

// Removes a block from a specific order's free list
static void list_remove(buddy_free_node* block, size_t order) {
    if (block->prev) {
        block->prev->next = block->next;
    } else {
        free_lists[order] = block->next;
    }
    
    if (block->next) {
        block->next->prev = block->prev;
    }
}

// Toggles the bit for a buddy pair and returns the NEW value of the bit.
// 0 means buddies are in the same state (both free/both used).
// 1 means they are in different states.
static bool toggle_buddy_bit(size_t pfn, size_t order) {
    size_t pair_index = pfn >> (order + 1);
    size_t byte_index = pair_index / 8;
    size_t bit_index = pair_index % 8;
    
    buddy_map[order][byte_index] ^= (1 << bit_index);
    
    return (buddy_map[order][byte_index] & (1 << bit_index)) != 0;
}

bool PMM::is_page_free(size_t target_pfn) {
    for (int order = 0; order <= mem::BUDDY_MAX_ORDER; order++) {
        buddy_free_node* curr = free_lists[order];
        while (curr) {
            size_t block_pfn = ((uintptr_t)curr - HHDM_BASE) >> PAGE_SHIFT;
            if (target_pfn >= block_pfn && target_pfn < block_pfn + (1ULL << order)) {
                return true;
            }
            curr = curr->next;
        }
    }
    return false;
}

bool PMM::allocate_specific_page(size_t target_pfn) {
    for (int order = 0; order <= mem::BUDDY_MAX_ORDER; order++) {
        buddy_free_node* curr = free_lists[order];
        while (curr) {
            size_t block_pfn = ((uintptr_t)curr - HHDM_BASE) >> PAGE_SHIFT;
            size_t pages_in_block = 1ULL << order;
            
            if (target_pfn >= block_pfn && target_pfn < block_pfn + pages_in_block) {
                list_remove(curr, order);
                toggle_buddy_bit(block_pfn, order);
                
                size_t current_order = order;
                size_t pfn = block_pfn;
                
                while (current_order > 0) {
                    current_order--;
                    size_t buddy_pfn = pfn ^ (1ULL << current_order);
                    
                    if (target_pfn >= buddy_pfn && target_pfn < buddy_pfn + (1ULL << current_order)) {
                        buddy_free_node* free_half = (buddy_free_node*)((pfn << PAGE_SHIFT) + HHDM_BASE);
                        list_add(free_half, current_order);
                        pfn = buddy_pfn;
                    } else {
                        buddy_free_node* free_half = (buddy_free_node*)((buddy_pfn << PAGE_SHIFT) + HHDM_BASE);
                        list_add(free_half, current_order);
                    }
                    toggle_buddy_bit(pfn, current_order);
                }
                
                used_memory += PAGE_SIZE;
                free_memory -= PAGE_SIZE;
                return true;
            }
            curr = curr->next;
        }
    }
    return false;
}

static size_t pages_to_order(size_t pages) {
    if (pages <= 1) return 0;
    
    // Find highest bit set, handling non-powers-of-two
    size_t order = 0;
    size_t count = pages - 1;
    while (count > 0) {
        count >>= 1;
        order++;
    }
    return order;
}

#pragma endregion



bool PMM::initialize_buddy(multiboot_tag* mmap) {
    if(mmap == nullptr) {
        kernel_panic("No mmap found to fully initialize PMM!\n");
        return false;
    }

    kprintf(gui::PrintTypes::LOG_INFO, "Initializing buddy allocator\n");

    // Initialize free lists
    for (int i = 0; i <= BUDDY_MAX_ORDER; i++) {
        free_lists[i] = nullptr;
    }

    // Calculating the max usable physical address using the mmap,
    // so we can set up the buddy system accordingly
    PhysAddr max_usable_addr = 0;
    MmapIterator iter(mmap);

    iter.for_each([&](const UnifiedMemoryEntry& ent) {
        // if(!ent.is_uefi || (ent.is_uefi && ent.is_usable))
        kprintf("   Found memory region 0x%x-0x%x Type: %s (Usable: %s)\n", ent.addr, ent.addr + ent.len, 
            ent.is_uefi ? mem_regions_uefi[ent.type] : mem_regions_bios[ent.type - 1], ent.is_usable ? "Yes" : "No");
        
        if (!ent.is_usable) return;

        total_memory += ent.len;
        PhysAddr entry_end = ent.addr + ent.len;
        if (entry_end > max_usable_addr) {
            max_usable_addr = entry_end;
        }
    });

    size_t total_pages = (max_usable_addr + PAGE_SIZE - 1) / PAGE_SIZE; // Max pages to cover this memory range
    if(total_pages == 0) {
        kernel_panic("Found no usable memory in mmap!\n");
        return false;
    }
    
    used_memory = total_memory;
    free_memory = 0;
    
    // Allocate the XOR Bitmaps right after the kernel
    size_t max_pfn = total_pages > 0 ? total_pages - 1 : 0;
    size_t total_bitmap_bytes = 0;
    
    for (int i = 0; i <= BUDDY_MAX_ORDER; i++) {
        size_t max_pair_index = max_pfn >> (i + 1);
        size_t num_pairs = max_pair_index + 1;
        
        size_t bytes_needed = (num_pairs + 7) / 8;
        if (bytes_needed == 0) bytes_needed = 1;
        
        total_bitmap_bytes += bytes_needed;
    }

    // I'll probably change this later to dynamically mark it in the bitmap (or something) because freeing GiBs of ram in early boot is slugish
    
    size_t bitmap_pages = align_up(total_bitmap_bytes, PAGE_SIZE) / PAGE_SIZE;
    uint8_t* bitmap_memory = (uint8_t*)((PhysAddr)alloc_pages_bump(bitmap_pages) + HHDM_BASE);
    memset(bitmap_memory, 0, bitmap_pages * PAGE_SIZE);
    
    size_t current_offset = 0;
    for (int i = 0; i <= BUDDY_MAX_ORDER; i++) {
        size_t max_pair_index = max_pfn >> (i + 1);
        size_t num_pairs = max_pair_index + 1;
        
        size_t bytes_needed = (num_pairs + 7) / 8;
        if (bytes_needed == 0) bytes_needed = 1;
        
        buddy_map[i] = bitmap_memory + current_offset;
        current_offset += bytes_needed;
    }
    
    // This is the physical address where safe, usable RAM actually begins (after kernel and the PMM bitmap)
    uintptr_t kernel_start = (uintptr_t)kernel_start_phys;
    uintptr_t kernel_mbi_end = align_up(highest_reserved_phys, PAGE_SIZE);
    
    iter.for_each([&](const UnifiedMemoryEntry& ent) {
        if (!ent.is_usable) return;

        PhysAddr region_start = align_up(ent.addr, PAGE_SIZE);
        PhysAddr region_end = align_down(ent.addr + ent.len, PAGE_SIZE);

        for (PhysAddr p = region_start; p < region_end; p += PAGE_SIZE) {
            // Protect address 0x0 (NULL pointer boundary)
            if (p == 0) continue;
            
            // Protect the Kernel and the newly generated XOR Bitmaps
            if (p >= kernel_start && p < kernel_mbi_end) continue;
            if (p >= highest_reserved_phys && p < bump_ptr_phys) continue;

            // Free the page into the buddy allocator
            free_pages((void*)p);
        }
    });
    
    initialized_buddy = true;
    kprintf(gui::PrintTypes::LOG_INFO, "Initialized buddy allocator. Total memory: %u B, free memory: %u B, used memory: %u B\n",
        get_total_memory(), get_free_memory(), get_used_memory());
    return true;
}


void* PMM::alloc_pages(size_t num) {
    if(!initialized_buddy) {
        return alloc_pages_bump(num);
    }

    size_t order = pages_to_order(num);

    if(order > mem::BUDDY_MAX_ORDER) return nullptr;
    
    size_t current_order = order;
    while (current_order <= mem::BUDDY_MAX_ORDER && free_lists[current_order] == nullptr) {
        current_order++;
    }

    if (current_order > mem::BUDDY_MAX_ORDER) { 
        kprintf(gui::PrintTypes::LOG_ERROR, "No memory to allocate (order: %u)\n", order); 
        return nullptr; 
    }

    buddy_free_node* block = free_lists[current_order];
    list_remove(block, current_order);

    size_t pfn = ((uintptr_t)block - HHDM_BASE) >> PAGE_SHIFT;

    toggle_buddy_bit(pfn, current_order);

    // Split the block down to the requested order
    while (current_order > order) {
        current_order--;
        
        // Calculate the address of the buddy we are splitting off
        size_t buddy_pfn = pfn ^ (1ULL << current_order);
        buddy_free_node* buddy = (buddy_free_node*)((buddy_pfn << PAGE_SHIFT) + HHDM_BASE);
        
        // Add the buddy to the lower order's free list
        list_add(buddy, current_order);
        
        // Toggle bit because one is now free and one is used
        toggle_buddy_bit(pfn, current_order);
    }

    used_memory += PAGE_SIZE * (1ULL << order);
    free_memory -= PAGE_SIZE * (1ULL << order);

    return (void*)(pfn << PAGE_SHIFT);
}

void PMM::free_pages(void* ptr, size_t num) {
    size_t order = pages_to_order(num);
    if(!ptr || order > mem::BUDDY_MAX_ORDER) return;
    
    size_t pfn = (uintptr_t)ptr >> PAGE_SHIFT;
    size_t original_order = order;

    while (order < mem::BUDDY_MAX_ORDER) {
        // Toggle the buddy bit
        bool bit_is_now_one = toggle_buddy_bit(pfn, order);

        // If the bit became 1, the buddy is used
        if (bit_is_now_one) {
            break;
        }

        // The bit became 0. Both buddies are free
        size_t buddy_pfn = pfn ^ (1ULL << order);
        buddy_free_node* buddy = (buddy_free_node*)((buddy_pfn << PAGE_SHIFT) + HHDM_BASE);

        // Remove the buddy from the current list
        list_remove(buddy, order);

        // The merged block's PFN is the aligned base of the two buddies
        pfn = pfn & ~(1ULL << order);
        order++;
    }

    // Add the final block (merged or unmerged) to its free list
    buddy_free_node* final_block = (buddy_free_node*)((pfn << PAGE_SHIFT) + HHDM_BASE);
    list_add(final_block, order);

    used_memory -= PAGE_SIZE * (1ULL << original_order);
    free_memory += PAGE_SIZE * (1ULL << original_order);
}


void PMM::mark_region_free(void* base, size_t length) {
    if (!initialized_buddy) return;
    PhysAddr start = align_up((PhysAddr)base, PAGE_SIZE);
    PhysAddr end = align_down((PhysAddr)base + length, PAGE_SIZE);

    for (PhysAddr p = start; p < end; p += PAGE_SIZE) {
        if (!is_page_free(p >> PAGE_SHIFT)) {
            free_pages((void*)p);
        }
    }
}

void PMM::mark_region_used(void* base, size_t length) {
    if (!initialized_buddy) return;
    PhysAddr start = align_down((PhysAddr)base, PAGE_SIZE);
    PhysAddr end = align_up((PhysAddr)base + length, PAGE_SIZE);

    for (PhysAddr p = start; p < end; p += PAGE_SIZE) {
        allocate_specific_page(p >> PAGE_SHIFT);
    }
}


// Statistics

size_t PMM::get_total_memory() {
    return total_memory;
}

size_t PMM::get_free_memory() {
    return free_memory;
}

size_t PMM::get_used_memory() {
    return used_memory;
}
