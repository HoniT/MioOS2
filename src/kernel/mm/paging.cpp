// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Bare metal x86_64 paging logic
// ========================================

#include <mm/paging.hpp>
#include <mm/pmm.hpp>
#include <graphics/kprint.hpp>
#include <cpu.hpp>

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




void PagingBackend::initialize() {
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
    klogf("VMM", "Initialized core Virtual Memory Manager\n");
}



#pragma region Core map & unmap API

PagingError PagingBackend::map_page(
        VirtAddr      virt,
        PhysAddr      phys,
        PageFlags     flags) noexcept {

    if (!is_page_aligned(virt))    return PagingError::InvalidAlignment;
    if (!is_page_aligned(phys))    return PagingError::InvalidAlignment;
    const bool user = has_flag(flags, PageFlags::User);
    const VirtualAddressFields va   = VirtualAddressFields::decompose(virt);
    
    // Level 4: PML4
    PageTable* pml4 = phys_to_virt(kernel_pml4_phys);
    PageTable* pdpt = get_or_create_subtable((*pml4)[va.pml4_index], user);
    if (!pdpt) return PagingError::OutOfMemory;
    
    // Level 3: PDPT
    auto& pdpt_entry = (*pdpt)[va.pdpt_index];
    if (pdpt_entry.is_present() && pdpt_entry.is_huge())
    return PagingError::AlreadyMapped;   // Covered by a 1 GiB huge page
    
    auto* pd = get_or_create_subtable(pdpt_entry, user);
    if (!pd) return PagingError::OutOfMemory;
    
    // Level 2: PD
    auto& pd_entry = (*pd)[va.pd_index];
    if (pd_entry.is_present() && pd_entry.is_huge())
    return PagingError::AlreadyMapped;   // Covered by a 2 MiB huge page
    
    auto* pt = get_or_create_subtable(pd_entry, user);
    if (!pt) return PagingError::OutOfMemory;
    
    // Level 1: PT (leaf)
    auto& pte = (*pt)[va.pt_index];
    if (pte.is_present()) return PagingError::AlreadyMapped;
    
    pte = PageTableEntry::make_page(phys, flags);
    if (!cpu::CPU::get_bsp_cpu().has_nx) {
        pte.bits.no_execute = 0;
    }
    flush_tlb_page(virt);
    return PagingError::Success;
}

PagingError PagingBackend::unmap_page(VirtAddr virt) noexcept {
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
