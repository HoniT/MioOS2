// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Bare metal x86_64 paging logic
// ========================================

#include <mm/paging.hpp>
#include <mm/pmm.hpp>
#include <mm/mmap_util.hpp>
#include <kernel_ui.hpp>
#include <cpu.hpp>
#include <lib/mem_util.hpp>
#include <kernel_panic.hpp>

using namespace mem;

PhysAddr PagingBackend::kernel_pml4_phys = 0;
bool PagingBackend::initialized = false;



void PagingBackend::write_cr3(uint64_t val) noexcept {
    asm volatile("mov %0, %%cr3" : : "r"(val) : "memory");
}

uint64_t PagingBackend::read_cr3() noexcept {
    uint64_t val;
    asm volatile("mov %%cr3, %0" : "=r"(val));
    return val;
}

void PagingBackend::invlpg(VirtAddr vaddr) noexcept {
    asm volatile("invlpg (%0)" : : "r"(vaddr) : "memory");
}

void PagingBackend::flush_tlb_page(VirtAddr virt) noexcept {
    invlpg(virt);
}

void PagingBackend::flush_tlb_full() noexcept {
    // Reload CR3 with itself; flushes all non-global TLB entries.
    write_cr3(read_cr3());
}



/// @brief Allocates a page table
PhysAddr PagingBackend::alloc_page_table() noexcept {
    PhysAddr pa = reinterpret_cast<PhysAddr>(PMM::alloc_pages());
    if (!pa) return 0;
    
    // Zero the page so every entry starts as not-present.
    phys_to_virt(pa)->clear();
    return pa;
}

PageTable* PagingBackend::get_or_create_subtable(
        PageTableEntry& entry, bool user) noexcept {
    if (entry.is_present()) {
        // Entry exists. If it's a huge-page leaf, the caller made a mistake.
        // Huge pages are detected by the caller before descending.
        return phys_to_virt(entry.physical_address());
    }

    // Allocate a new page table for the next level.
    PhysAddr child_phys = alloc_page_table();
    if (!child_phys) return nullptr;
    
    entry = PageTableEntry::make_table(child_phys, user);
    return phys_to_virt(child_phys);
}




void PagingBackend::initialize(multiboot_tag* mmap) {
    if (!mmap) kernel_panic("No mmap provided to initialize the core Virtual Memory Manager\n");

    // Enable NX bit support in the CPU if available
    if(cpu::CPU::get_bsp_cpu().has_nx) {
        uint64_t efer = cpu::CPU::read_msr(0xC0000080);
        efer |= (1ULL << 11);
        cpu::CPU::write_msr(0xC0000080, efer);
    }

    uint64_t p4 = read_cr3() & ~0xFFF;
    kernel_pml4_phys = p4;
    auto* pml4 = kernel_pml4();
    // Clear the identity map
    (*pml4)[0] = PageTableEntry::make_empty();
    flush_tlb_full();
    
    initialized = true;

    // Map the memory into the HHDM safely by iterating over usable regions
    MmapIterator iter(mmap);
    PhysAddr max_usable_addr = 0;

    iter.for_each([&](const UnifiedMemoryEntry& ent) {
        // if (!ent.is_usable) return;

        PhysAddr entry_end = ent.addr + ent.len;
        if (entry_end > max_usable_addr) {
            max_usable_addr = entry_end;
        }

        // Align region bounds to page size
        PhysAddr region_start = align_up(ent.addr, PAGE_SIZE);
        PhysAddr region_end = align_down(ent.addr + ent.len, PAGE_SIZE);

        if (region_start >= region_end) return;

        // Only map portions that lie beyond the early boot limit 
        // (everything below EARLY_BOOT_MAP_LIMIT is already HHDM mapped by boot.asm)
        PhysAddr map_start = (region_start > EARLY_BOOT_MAP_LIMIT) ? region_start : EARLY_BOOT_MAP_LIMIT;
        
        for (PhysAddr p = map_start; p < region_end; p += PAGE_SIZE) {
            map_page(p + HHDM_BASE, p, PageFlags::KernelRW); 
        }
    });

    flush_tlb_full();
    
    PhysAddr end_phys = align_up(max_usable_addr, PAGE_SIZE);
    kprintf(gui::PrintTypes::LOG_INFO, "Mapped entire physical RAM into HHDM (up to %u MiB)\n", end_phys / 1048576);
    kprintf(gui::PrintTypes::LOG_INFO, "Initialized core Virtual Memory Manager\n");
}



#pragma region Core map & unmap API

PagingError PagingBackend::map_page(
        VirtAddr      virt,
        PhysAddr      phys,
        PageFlags     flags) noexcept {

    if (!is_page_aligned(virt))    return PagingError::InvalidAlignment;
    if (!is_page_aligned(phys))    return PagingError::InvalidAlignment;
    
    const bool user = has_flag(flags, PageFlags::User);
    const VirtualAddressFields va = VirtualAddressFields::decompose(virt);
    
    // Level 4: PML4
    PageTable* pml4 = phys_to_virt(kernel_pml4_phys);
    PageTable* pdpt = get_or_create_subtable((*pml4)[va.pml4_index], user);
    if (!pdpt) return PagingError::OutOfMemory;
    
    // Level 3: PDPT
    auto& pdpt_entry = (*pdpt)[va.pdpt_index];
    if (pdpt_entry.is_present() && pdpt_entry.is_huge()) {
        return PagingError::AlreadyMapped;
    }

    // Check for 1 GiB Huge Page mapping
    if (has_flag(flags, PageFlags::Huge1G)) {
        if (!cpu::CPU::get_bsp_cpu().has_pdpe1gb) {
            return PagingError::HardwareFault; 
        }
        if (pdpt_entry.is_present()) {
            return PagingError::AlreadyMapped; // Cannot overwrite existing directory
        }

        pdpt_entry = PageTableEntry::make_huge_1g(phys, flags);
        if (!cpu::CPU::get_bsp_cpu().has_nx) pdpt_entry.bits.no_execute = 0;
        return PagingError::Success;
    }
    
    auto* pd = get_or_create_subtable(pdpt_entry, user);
    if (!pd) return PagingError::OutOfMemory;
    
    // Level 2: PD
    auto& pd_entry = (*pd)[va.pd_index];
    if (pd_entry.is_present() && pd_entry.is_huge()) {
        return PagingError::AlreadyMapped;
    }

    // Check for 2 MiB Huge Page mapping
    if (has_flag(flags, PageFlags::Huge2M)) {
        if (pd_entry.is_present()) {
            return PagingError::AlreadyMapped; // Cannot overwrite existing directory
        }

        pd_entry = PageTableEntry::make_huge_2m(phys, flags);
        if (!cpu::CPU::get_bsp_cpu().has_nx) pd_entry.bits.no_execute = 0;
        return PagingError::Success;
    }
    
    auto* pt = get_or_create_subtable(pd_entry, user);
    if (!pt) return PagingError::OutOfMemory;
    
    // Level 1: PT (leaf)
    auto& pte = (*pt)[va.pt_index];
    if (pte.is_present()) return PagingError::AlreadyMapped;
    
    pte = PageTableEntry::make_page(phys, flags);
    if (!cpu::CPU::get_bsp_cpu().has_nx) {
        pte.bits.no_execute = 0;
    }
    
    return PagingError::Success;
}

PagingError PagingBackend::unmap_page(VirtAddr virt) noexcept {
    if (!is_page_aligned(virt)) return PagingError::InvalidAlignment;
    const auto va = VirtualAddressFields::decompose(virt);
    auto* pml4 = phys_to_virt(kernel_pml4_phys);

    auto& pml4e = (*pml4)[va.pml4_index];
    if (!pml4e.is_present()) return PagingError::NotMapped;

    auto* pdpt = phys_to_virt(pml4e.physical_address());
    auto& pdpte = (*pdpt)[va.pdpt_index];
    if (!pdpte.is_present()) return PagingError::NotMapped;
    
    // Check for 1 GiB huge page
    if (pdpte.is_huge()) {
        pdpte = PageTableEntry::make_empty();
        flush_tlb_page(virt);
        return PagingError::Success;
    }

    auto* pd = phys_to_virt(pdpte.physical_address());
    auto& pde = (*pd)[va.pd_index];
    if (!pde.is_present()) return PagingError::NotMapped;
    
    // Check for 2 MiB huge page
    if (pde.is_huge()) {
        pde = PageTableEntry::make_empty();
        flush_tlb_page(virt);
        return PagingError::Success;
    }

    auto* pt = phys_to_virt(pde.physical_address());
    auto& pte = (*pt)[va.pt_index];
    if (!pte.is_present()) return PagingError::NotMapped;

    pte = PageTableEntry::make_empty();
    flush_tlb_page(virt);
    return PagingError::Success;
}

PagingError PagingBackend::protect_page(VirtAddr virt, PageFlags new_flags) noexcept {

    if (!is_page_aligned(virt)) return PagingError::InvalidAlignment;

    const auto va = VirtualAddressFields::decompose(virt);

    auto* pml4 = phys_to_virt(kernel_pml4_phys);
    if (!(*pml4)[va.pml4_index].is_present()) return PagingError::NotMapped;
    auto* pdpt = phys_to_virt((*pml4)[va.pml4_index].physical_address());
    if (!(*pdpt)[va.pdpt_index].is_present()) return PagingError::NotMapped;
    auto* pd = phys_to_virt((*pdpt)[va.pdpt_index].physical_address());
    if (!(*pd)[va.pd_index].is_present())     return PagingError::NotMapped;
    auto* pt = phys_to_virt((*pd)[va.pd_index].physical_address());
    auto& pte = (*pt)[va.pt_index];
    if (!pte.is_present()) return PagingError::NotMapped;

    // Preserve the physical address; replace the permission bits.
    PhysAddr phys = pte.physical_address();
    pte = PageTableEntry::make_page(phys, new_flags);
    if (!cpu::CPU::get_bsp_cpu().has_nx) {
        pte.bits.no_execute = 0;
    }
    invlpg(virt);
    return PagingError::Success;
}

PhysAddr PagingBackend::translate(VirtAddr virt) noexcept {
    const auto va = VirtualAddressFields::decompose(virt);

    auto* pml4 = phys_to_virt(kernel_pml4_phys);
    const auto& e4 = (*pml4)[va.pml4_index];
    if (!e4.is_present()) return 0;

    auto* pdpt = phys_to_virt(e4.physical_address());
    const auto& e3 = (*pdpt)[va.pdpt_index];
    if (!e3.is_present()) return 0;
    if (e3.is_huge()) {
        // 1 GiB page: physical base + offset within 1 GiB page.
        return e3.physical_address() | (virt & (PAGE_SIZE_1G - 1));
    }

    auto* pd = phys_to_virt(e3.physical_address());
    const auto& e2 = (*pd)[va.pd_index];
    if (!e2.is_present()) return 0;
    if (e2.is_huge()) {
        // 2 MiB page: physical base + offset within 2 MiB page.
        return e2.physical_address() | (virt & (PAGE_SIZE_2M - 1));
    }

    auto* pt = phys_to_virt(e2.physical_address());
    const auto& e1 = (*pt)[va.pt_index];
    if (!e1.is_present()) return 0;

    // 4 KiB page: physical base + 12-bit page offset.
    return e1.physical_address() | va.page_offset;
}

#pragma endregion
