// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// GRUB Multiboot2 helper methods
// ========================================

#include <boot/multiboot.hpp>

void* Multiboot2::find_tag(void* mb2_info, uint32_t tag_type) {
    multiboot2_info* info = (multiboot2_info*)mb2_info;
    multiboot_tag* tag = (multiboot_tag*)((uint8_t*)mb2_info + 8);
    
    while (tag->type != MULTIBOOT_TAG_TYPE_END) {
        if (tag->type == tag_type) {
            return tag;
        }
        
        // Tags are 8-byte aligned
        tag = (multiboot_tag*)((uint8_t*)tag + ((tag->size + 7) & ~7));
        
        // Safety check
        if ((uint8_t*)tag >= (uint8_t*)mb2_info + info->total_size) {
            break;
        }
    }
    
    return nullptr;
}
    
multiboot_tag_framebuffer* Multiboot2::get_framebuffer(void* mb2_info) {
    return (multiboot_tag_framebuffer*)find_tag(mb2_info, MULTIBOOT_TAG_TYPE_FRAMEBUFFER);
}

multiboot_tag_basic_meminfo* Multiboot2::get_basic_meminfo(void* mb2_info) {
    return (multiboot_tag_basic_meminfo*)find_tag(mb2_info, MULTIBOOT_TAG_TYPE_BASIC_MEMINFO);
}

multiboot_tag_string* Multiboot2::get_cmdline(void* mb2_info) {
    return (multiboot_tag_string*)find_tag(mb2_info, MULTIBOOT_TAG_TYPE_CMDLINE);
}

multiboot_tag_string* Multiboot2::get_bootloader_name(void* mb2_info) {
    return (multiboot_tag_string*)find_tag(mb2_info, MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME);
}

multiboot_tag_acpi* Multiboot2::get_acpi_new(void* mb2_info) {
    return (multiboot_tag_acpi*)find_tag(mb2_info, MULTIBOOT_TAG_TYPE_ACPI_NEW);
}

multiboot_tag_acpi* Multiboot2::get_acpi_old(void* mb2_info) {
    return (multiboot_tag_acpi*)find_tag(mb2_info, MULTIBOOT_TAG_TYPE_ACPI_OLD);
}

multiboot_tag* Multiboot2::get_mmap(void* mbi) {
    // Try to find the EFI Memory Map first
    multiboot_tag* efi_mmap = (multiboot_tag*)find_tag(mbi, MULTIBOOT_TAG_TYPE_EFI_MMAP);
    if (efi_mmap != nullptr) {
        return efi_mmap;
    }
    // Fall back to the standard BIOS Memory Map
    multiboot_tag* bios_mmap = (multiboot_tag*)find_tag(mbi, MULTIBOOT_TAG_TYPE_MMAP);
    if (bios_mmap != nullptr) {
        return bios_mmap;
    }
    return nullptr;
}

multiboot_tag_bootdev* Multiboot2::get_bootdev(void* mb2_info) {
    return (multiboot_tag_bootdev*)find_tag(mb2_info, MULTIBOOT_TAG_TYPE_BOOTDEV);
}
