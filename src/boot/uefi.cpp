// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// UEFI helper methods
// ========================================

#include <boot/uefi.hpp>
#include <boot/multiboot.hpp>
#include <arch/acpi/rsdp.hpp>
#include <mm/mm_defs.hpp>

bool uefi::compare_guid(const EFI_GUID& a, const EFI_GUID& b) {
    if (a.Data1 != b.Data1 || a.Data2 != b.Data2 || a.Data3 != b.Data3) return false;
    for (int i = 0; i < 8; i++) {
        if (a.Data4[i] != b.Data4[i]) return false;
    }
    return true;
}

void* uefi::scan_uefi_for_rsdp(void* mb2_info) {
    multiboot_tag_efi64* efi_tag = Multiboot2::get_efi64(mb2_info);
    if (efi_tag == nullptr) {
        return nullptr; // Not booted via UEFI, or tag missing
    }

    EFI_SYSTEM_TABLE* sys_table = (EFI_SYSTEM_TABLE*)(efi_tag->pointer + mem::HHDM_BASE);
    // ACPI 2.0 GUID: 8868E871-E4F1-11D3-BC22-0080C73C8881
    EFI_GUID acpi20_guid = {0x8868E871, 0xE4F1, 0x11D3, {0xBC, 0x22, 0x00, 0x80, 0xC7, 0x3C, 0x88, 0x81}};
    
    // ACPI 1.0 GUID: EB9D2D30-2D88-11D3-9A16-0090273FC14D
    EFI_GUID acpi10_guid = {0xEB9D2D30, 0x2D88, 0x11D3, {0x9A, 0x16, 0x00, 0x90, 0x27, 0x3F, 0xC1, 0x4D}};

    EFI_CONFIGURATION_TABLE* config_table = (EFI_CONFIGURATION_TABLE*)(sys_table->ConfigurationTable + mem::HHDM_BASE);
    rsdp_descriptor* fallback_rsdp = nullptr;

    // Iterate through the UEFI Configuration Tables
    for (uint64_t i = 0; i < sys_table->NumberOfTableEntries; i++) {
        if (compare_guid(config_table[i].VendorGuid, acpi20_guid)) {
            // Found ACPI 2.0+, prioritize this and return immediately
            rsdp_descriptor* rsdp = (rsdp_descriptor*)(config_table[i].VendorTable + mem::HHDM_BASE);
            if (acpi::RSDP::is_valid_rsdp(rsdp)) return rsdp;
        } 
        else if (compare_guid(config_table[i].VendorGuid, acpi10_guid)) {
            // Found ACPI 1.0. Save it, but keep searching in case ACPI 2.0 exists further down
            rsdp_descriptor* rsdp = (rsdp_descriptor*)(config_table[i].VendorTable + mem::HHDM_BASE);
            if (acpi::RSDP::is_valid_rsdp(rsdp)) fallback_rsdp = rsdp;
        }
    }

    return fallback_rsdp;
}
