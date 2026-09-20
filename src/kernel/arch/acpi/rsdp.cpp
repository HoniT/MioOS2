// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// ACPI Root / Extended System Description Pointer methods
// ========================================

#include <arch/acpi/rsdp.hpp>
#include <arch/acpi/acpi.hpp>
#include <lib/mem_util.hpp>
#include <boot/multiboot.hpp>
#include <kernel_ui.hpp>
#include <kernel_panic.hpp>
#include <mm/mm_defs.hpp>
#include <boot/uefi.hpp>

using namespace acpi;

sdp_descriptor* SystemDescriptionPointer::sdp = nullptr;

bool SystemDescriptionPointer::is_valid_sdp(const sdp_descriptor* sdp) {
    int res = memcmp(sdp, "RSD PTR ", 8);
    if(res != 0) return false;
    
    
    // ACPI 1.0 Checksum (First 20 bytes)
    uint8_t sum = 0;
    const uint8_t* ptr = (const uint8_t*)sdp;
    
    for (int i = 0; i < 20; i++) {
        sum += ptr[i];
    }
    
    if (sum != 0) {
        return false; // Fails ACPI 1.0 validation
    }

    // ACPI 2.0+ Extended Checksum
    if (sdp->revision >= 2) {
        uint8_t extended_sum = 0;
        
        for (uint32_t i = 0; i < sdp->length; i++) {
            extended_sum += ptr[i];
        }
        
        if (extended_sum != 0) {
            return false; // Fails ACPI 2.0+ extended validation
        }
    }

    return true;
}

bool SystemDescriptionPointer::is_valid_sdt_ent(acpi_header_t* table) {
    if (!table) return false;
    
    uint8_t sum = 0;
    uint8_t* ptr = (uint8_t*)table;
    
    for (uint32_t i = 0; i < table->length; i++) {
        sum += ptr[i];
    }
    
    return (sum == 0);
}

sdp_descriptor* SystemDescriptionPointer::scan_memory_for_sdp(uintptr_t start_phys, uintptr_t end_phys) {
    for (uintptr_t addr = start_phys; addr < end_phys; addr += 16) {
        sdp_descriptor* sdp = (sdp_descriptor*)(addr + mem::HHDM_BASE);
        if (is_valid_sdp(sdp)) {
            return sdp;
        }
    }
    return nullptr;
}

sdp_descriptor* SystemDescriptionPointer::find_sdp(void* mb2_info) {
    if(!mb2_info || acpi_reclaimed) return nullptr;

    multiboot_tag_acpi* acpi_new = Multiboot2::get_acpi_new(mb2_info);
    if (acpi_new != nullptr && is_valid_sdp((sdp_descriptor*)&acpi_new->rsdp[0])) {
        SystemDescriptionPointer::sdp = (sdp_descriptor*)&acpi_new->rsdp[0];
        kprintf(gui::LOG_INFO, "Found the new RSDP/XDP at 0x%x from Multiboot2\n", sdp);
        return (sdp_descriptor*)&acpi_new->rsdp[0];
    }

    // Try ACPI 1.0 via Multiboot2
    multiboot_tag_acpi* acpi_old = Multiboot2::get_acpi_old(mb2_info);
    if (acpi_old != nullptr && is_valid_sdp((sdp_descriptor*)&acpi_old->rsdp[0])) {
        SystemDescriptionPointer::sdp = (sdp_descriptor*)&acpi_old->rsdp[0];
        kprintf(gui::LOG_INFO, "Found the old RSDP/XDP at 0x%x from Multiboot2\n", sdp);
        return (sdp_descriptor*)&acpi_old->rsdp[0];
    }

    // Fallback to manual finding with UEFI
    sdp_descriptor* sdp = (sdp_descriptor*)uefi::scan_uefi_for_rsdp(mb2_info);
    if (sdp != nullptr) {
        SystemDescriptionPointer::sdp = sdp;
        kprintf(gui::LOG_INFO, "Found the RSDP/XDP at 0x%x from the UEFI configuration tables\n", sdp);
        return sdp;
    }

    // Fallback to manual memory scanning (IA-PC)
    
    // Scan Extended BIOS Data Area
    uint16_t* ebda_ptr = (uint16_t*)(0x040E + mem::HHDM_BASE);
    uintptr_t ebda_address = (*ebda_ptr) << 4;
    sdp = scan_memory_for_sdp(ebda_address, ebda_address + 1024);
    if (sdp != nullptr) {
        SystemDescriptionPointer::sdp = sdp;
        kprintf(gui::LOG_INFO, "Found the RSDP/XDP at 0x%x by scanning the Extended BIOS Area\n", sdp);
        return sdp;
    }

    // Scan Main BIOS Area (0x000E0000 to 0x000FFFFF)
    sdp = scan_memory_for_sdp(0x000E0000, 0x000FFFFF);
    if(sdp != nullptr) {
        SystemDescriptionPointer::sdp = sdp;
        kprintf(gui::LOG_INFO, "Found the RSDP/XDP at 0x%x by scanning the Main BIOS Area\n", sdp);
        return sdp;
    }

    kernel_panic("Couldn't find the RSDP/XDP\n");
    return nullptr;
}

acpi_header_t* SystemDescriptionPointer::find_table_by_signature(char signature[4]) {
    if(!sdp || !signature || acpi_reclaimed) return nullptr;

    
    if(sdp->revision >= 2) {
        xsdt_t* xsdt = (xsdt_t*)(sdp->xsdt_address + mem::HHDM_BASE);
        if (xsdt->length < sizeof(acpi_header_t)) return nullptr;

        uint32_t entries = (xsdt->length - sizeof(acpi_header_t)) / 8;
        for (uint32_t i = 0; i < entries; i++) {
            acpi_header_t* current_table = (acpi_header_t*)(xsdt->pointers[i] + mem::HHDM_BASE);
            
            // Compare the signature
            if (memcmp(current_table->signature, signature, 4) == 0) {
                return current_table;
            }
        }
        return nullptr;
    }
    else {
        rsdt_t* rsdt = (rsdt_t*)(sdp->rsdt_address + mem::HHDM_BASE);
        if (rsdt->length < sizeof(acpi_header_t)) return nullptr;
        
        uint32_t entries = (rsdt->length - sizeof(acpi_header_t)) / 4;
        for (uint32_t i = 0; i < entries; i++) {
            acpi_header_t* current_table = (acpi_header_t*)(rsdt->pointers[i] + mem::HHDM_BASE);
            
            // Compare the signature
            if (memcmp(current_table->signature, signature, 4) == 0) {
                return current_table;
            }
        }
    }
    return nullptr;
}

void SystemDescriptionPointer::parse_sdt(util::List<acpi_header_t*>& list) {
    if(!sdp || acpi_reclaimed) return;

    if(sdp->revision >= 2) {
        xsdt_t* xsdt = (xsdt_t*)(sdp->xsdt_address + mem::HHDM_BASE);
        if (xsdt->length < sizeof(acpi_header_t)) return;

        size_t entries = (xsdt->length - sizeof(acpi_header_t)) / 8; 
        for (size_t i = 0; i < entries; i++) {
            acpi_header_t* table = (acpi_header_t*)(xsdt->pointers[i] + mem::HHDM_BASE); 
            
            if (is_valid_sdt_ent(table)) {
                list.push_front(table);
            } else {
                kprintf(gui::LOG_ERROR, "Corrupt ACPI table at %p\n", table);
            }
        }
    }
    else {
        rsdt_t* rsdt = (rsdt_t*)(sdp->rsdt_address + mem::HHDM_BASE);
        if (rsdt->length < sizeof(acpi_header_t)) return;

        size_t entries = (rsdt->length - sizeof(acpi_header_t)) / 4; 
        for (size_t i = 0; i < entries; i++) {
            acpi_header_t* table = (acpi_header_t*)(rsdt->pointers[i] + mem::HHDM_BASE); 
            
            if (is_valid_sdt_ent(table)) {
                list.push_front(table);
            } else {
                kprintf(gui::LOG_ERROR, "Corrupt ACPI table at %p\n", table);
            }
        }
    }
}
