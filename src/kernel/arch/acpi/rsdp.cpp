// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// ACPI Root System Description Pointer methods
// ========================================

#include <arch/acpi/rsdp.hpp>
#include <lib/mem_util.hpp>
#include <boot/multiboot.hpp>
#include <graphics/kernel_gui.hpp>
#include <kernel_panic.hpp>
#include <mm/mm_defs.hpp>

using namespace acpi;

rsdp_descriptor* RSDP::rsdp = nullptr;

bool RSDP::is_valid_rsdp(const rsdp_descriptor* rsdp) {
    int res = memcmp(rsdp, "RSD PTR ", 8);
    if(res != 0) return false;
    
    
    // ACPI 1.0 Checksum (First 20 bytes)
    uint8_t sum = 0;
    const uint8_t* ptr = (const uint8_t*)rsdp;
    
    for (int i = 0; i < 20; i++) {
        sum += ptr[i];
    }
    
    if (sum != 0) {
        return false; // Fails ACPI 1.0 validation
    }

    // ACPI 2.0+ Extended Checksum
    if (rsdp->revision >= 2) {
        uint8_t extended_sum = 0;
        
        for (uint32_t i = 0; i < rsdp->length; i++) {
            extended_sum += ptr[i];
        }
        
        if (extended_sum != 0) {
            return false; // Fails ACPI 2.0+ extended validation
        }
    }

    return true;
}

rsdp_descriptor* RSDP::scan_memory_for_rsdp(uintptr_t start_phys, uintptr_t end_phys) {
    for (uintptr_t addr = start_phys; addr < end_phys; addr += 16) {
        rsdp_descriptor* rsdp = (rsdp_descriptor*)(addr + mem::HHDM_BASE);
        if (is_valid_rsdp(rsdp)) {
            return rsdp;
        }
    }
    return nullptr;
}

rsdp_descriptor* RSDP::find_rsdp(void* mb2_info) {
    multiboot_tag_acpi* acpi_new = Multiboot2::get_acpi_new(mb2_info);
    if (acpi_new != nullptr) {
        RSDP::rsdp = (rsdp_descriptor*)&acpi_new->rsdp[0];
        kprintf(gui::LOG_INFO, "Found the new RSDP at 0x%x from Multiboot2\n", rsdp);
        return (rsdp_descriptor*)&acpi_new->rsdp[0];
    }

    // Try ACPI 1.0 via Multiboot2
    multiboot_tag_acpi* acpi_old = Multiboot2::get_acpi_old(mb2_info);
    if (acpi_old != nullptr) {
        RSDP::rsdp = (rsdp_descriptor*)&acpi_old->rsdp[0];
        kprintf(gui::LOG_INFO, "Found the old RSDP at 0x%x from Multiboot2\n", rsdp);
        return (rsdp_descriptor*)&acpi_old->rsdp[0];
    }

    // Fallback to manual memory scanning (IA-PC)
    
    // Scan Extended BIOS Data Area
    uint16_t* ebda_ptr = (uint16_t*)(0x040E + mem::HHDM_BASE);
    uintptr_t ebda_address = (*ebda_ptr) << 4;
    rsdp_descriptor* rsdp = scan_memory_for_rsdp(ebda_address, ebda_address + 1024);
    if (rsdp != nullptr) {
        RSDP::rsdp = rsdp;
        kprintf(gui::LOG_INFO, "Found the RSDP at 0x%x by scanning the Extended BIOS Area\n", rsdp);
        return rsdp;
    }

    // Scan Main BIOS Area (0x000E0000 to 0x000FFFFF)
    rsdp = scan_memory_for_rsdp(0x000E0000, 0x000FFFFF);
    if(rsdp != nullptr) {
        RSDP::rsdp = rsdp;
        kprintf(gui::LOG_INFO, "Found the RSDP at 0x%x by scanning the Main BIOS Area\n", rsdp);
        return rsdp;
    }


    // Fallback to manual finding with UEFI

    

    kernel_panic("Couldn't find the RSDP\n");
    return nullptr;
}